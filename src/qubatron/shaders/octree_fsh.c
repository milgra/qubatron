#version 300 es
/* #define OCTTEST 1 */

precision mediump float;

in vec2  coord;
out vec4 fragColor;

uniform vec3 camfp;
uniform vec3 angle_in;
uniform vec3 light;
uniform vec4 basecube;
uniform vec2 dimensions;
uniform int  maxlevel;
uniform int  shoot;

/* highp vec3 light = vec3(0.0, 2000.0, -500.0); // dynamic light */

// octets
//     4 5
//     6 7
// 0 1
// 2 3

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0); // x shift
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0); // y shift
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0); // z shift

const int horpairs[] = int[8](1, 0, 3, 2, 5, 4, 7, 6);
const int verpairs[] = int[8](2, 3, 0, 1, 6, 7, 4, 5);
const int deppairs[] = int[8](4, 5, 6, 7, 0, 1, 2, 3);

uniform sampler2D coltexbuf_s; // color data per point for static model
uniform sampler2D coltexbuf_d; // color data per point for dynamic model

uniform sampler2D nrmtexbuf_s; // normal data per point for static
uniform sampler2D nrmtexbuf_d; // normal data per point for dynamic

uniform lowp isampler2D octtexbuf_s; // octree for static in a format, nine ints, first 8 is octets for cube, 9th is index for next octet/color/normal
uniform lowp isampler2D octtexbuf_d; // octree for dynamic

struct ctlres
{
    vec4 isp;
    vec4 col;
    vec4 nrm;
    vec4 tlf;
};

struct stck_t
{
    vec4 cube;    // cube dimensions for stack level
    vec4 isps[4]; // is points for stack level cube
    int  octs[4]; // octet numbers for isps
    int  ispsi;   // | 0 level checked | 1 isps ind0 | 2 isps ind1 | 3 isps ind2 | 4 | 5 ispsp len0 | 6 isps len1 | 7 isps len2 |
    int  socti;   // static cube octets index for stack level
    int  docti;   // dynamic cube octets index for stack level
};

#define FLT_MAX 3.402823466e+38

vec4 is_cube_xplane(float x, vec3 lp, vec3 lv)
{
    vec4 r = vec4(FLT_MAX);
    if (lv.x != 0.0)
    {
	r.w = (x - lp.x) / lv.x;
	r.y = lp.y + lv.y * r.w;
	r.z = lp.z + lv.z * r.w;
	r.x = x;
    }
    return r;
}

vec4 is_cube_yplane(float y, vec3 lp, vec3 lv)
{
    vec4 r = vec4(FLT_MAX);
    if (lv.y != 0.0)
    {
	r.w = (y - lp.y) / lv.y;
	r.x = lp.x + lv.x * r.w;
	r.z = lp.z + lv.z * r.w;
	r.y = y;
    }
    return r;
}

vec4 is_cube_zplane(float z, vec3 lp, vec3 lv)
{
    vec4 r = vec4(FLT_MAX);
    if (lv.z != 0.0)
    {
	r.w = (z - lp.z) / lv.z;
	r.x = lp.x + lv.x * r.w;
	r.y = lp.y + lv.y * r.w;
	r.z = z;
    }
    return r;
}

float random(vec3 pos)
{
    return fract(sin(dot(pos, vec3(64.25375463, 23.27536534, 86.29678483))) * 59482.7542);
}

/* int[9] octets_for_index(int i, lowp isampler2D sampler) */
/* { */
/*     i *= 3; // octet is 12 byte long in octet texture */

/*     int cy = i / 8192; */
/*     int cx = i - cy * 8192; */

/*     ivec4 p0, p1, p2; */

/*     /\* ivec2 ts = textureSize(octtexbuf_s, 0); *\/ */
/*     /\* if (cx <= ts.x && cy <= ts.y) *\/ */
/*     /\* { *\/ */
/*     p0 = texelFetch(sampler, ivec2(cx, cy), 0); */
/*     p1 = texelFetch(sampler, ivec2(cx + 1, cy), 0); */
/*     p2 = texelFetch(sampler, ivec2(cx + 2, cy), 0); */

/*     return int[9](p0.x, p0.y, p0.z, p0.w, p1.x, p1.y, p1.z, p1.w, p2.x); */
/* } */

// !!! TODO octet texture and index texture buffer should be separate to save size and bandwidth

int oct_from_octets_for_index(int octi, int i, lowp isampler2D sampler, int level)
{
    if (i == 0 && level > 0) return 0;
    int elem = i * 3 + octi / 4; // octet is 12 byte long in octet texture

    int cy = elem / 8192;
    int cx = elem - cy * 8192;

    return texelFetch(sampler, ivec2(cx, cy), 0)[octi - (octi / 4) * 4];
}

ctlres
cube_trace_line(vec3 pos, vec3 dir)
{
    int level = 0;

    ctlres res;
    res.isp = vec4(0.0, 0.0, 0.0, 0.0);
#ifdef OCTTEST
    res.col = vec4(0.0, 0.2, 0.0, 0.0);
#else
    res.col = vec4(0.0, 0.0, 0.0, 0.0);
#endif
    // under 17 and over 20 shader becomes very slow, why?
    stck_t stck[18];
    stck[0].cube  = basecube;
    stck[0].socti = 0;
    stck[0].docti = 0;
    stck[0].ispsi = 0;

    vec4 act;
    vec4 tlf = basecube;
    vec4 brb = vec4(tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0);

    int  hitc = 0;
    vec4 hitp[4];

    // front side
    act = is_cube_zplane(tlf.z, pos, dir);
    if (tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
	hitp[hitc++] = act;

    // back side
    act = is_cube_zplane(brb.z, pos, dir);
    if (tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
	hitp[hitc++] = act;

    // left side
    act = is_cube_xplane(tlf.x, pos, dir);
    if (tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // right side
    act = is_cube_xplane(brb.x, pos, dir);
    if (tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // top side
    act = is_cube_yplane(tlf.y, pos, dir);
    if (tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // bottom side
    act = is_cube_yplane(brb.y, pos, dir);
    if (tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
	hitp[hitc++] = act;

    // we don't deal with the outside world
    if (hitc < 2) discard;

    // one hitp have to be in front of the camera
    if (hitp[0].w < 0.0 && hitp[1].w < 0.0) discard;

#ifdef OCTTEST
    res.col.a += 0.2;
#endif

    // order side hitpoints
    if (hitp[1].w < hitp[0].w) hitp[0] = hitp[1];

    // inside cube, starting isp will be pos
    if (hitp[0].w < 0.0) hitp[0] = vec4(pos, 0.0);

    // store starting isp
    stck[level].isps[0] = hitp[0];

    while (true)
    {
	tlf = stck[level].cube;

	// return if we reached bottom
	if (level == maxlevel)
	{
	    res.isp = stck[level].isps[0];
	    res.tlf = tlf;

	    int socti = oct_from_octets_for_index(8, stck[level].socti, octtexbuf_s, level);
	    int docti = oct_from_octets_for_index(8, stck[level].docti, octtexbuf_d, level);

	    int   cy  = socti / 8192;
	    int   cx  = socti - cy * 8192;
	    ivec2 crd = ivec2(cx, cy);

	    res.col = texelFetch(coltexbuf_s, crd, 0);
	    res.nrm = texelFetch(nrmtexbuf_s, crd, 0);

	    if (docti > 0)
	    {
		cy  = docti / 8192;
		cx  = docti - cy * 8192;
		crd = ivec2(cx, cy);

		res.col = texelFetch(coltexbuf_d, crd, 0);
		res.nrm = texelFetch(nrmtexbuf_d, crd, 0);
	    }

	    // add sub-detail fuzzyness SWITCHABLE
	    /* if (res.isp.w < 1.0) */
	    /* 	res.col.xyz += res.col.xyz * random(res.isp.xyz) * 0.4; */

	    return res;
	}

	// check subnode intersection if needed
	if (stck[level].ispsi == 0)
	{
	    stck[level].ispsi = 128;

	    vec4 brb = vec4(tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0);
	    vec4 hlf = brb + (tlf - brb) * 0.5;

	    hitc    = 1;
	    hitp[0] = stck[level].isps[0];

	    // z div plane
	    act = is_cube_zplane(hlf.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y) hitp[hitc++] = act;

	    // x div plane
	    act = is_cube_xplane(hlf.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z) hitp[hitc++] = act;

	    // y div plane
	    act = is_cube_yplane(hlf.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z) hitp[hitc++] = act;

	    int oct = 0;
	    int pre = -1;

	    for (int i = 0; i < hitc; ++i)
	    {
		// order hitpoints
		if (i < hitc - 1)
		{
		    for (int j = i + 1; j < hitc; ++j)
		    {
			if (hitp[j].w < hitp[i].w)
			{
			    act     = hitp[i];
			    hitp[i] = hitp[j];
			    hitp[j] = act;
			}
		    }
		}

		act = hitp[i];
		oct = 0;

		// calculate octet number for current smallest
		if (act.x > hlf.x) oct = 1;
		if (act.y < hlf.y) oct += 2;
		if (act.z < hlf.z) oct += 4;

		// from the second isp find octet pairs if needed
		if (oct == pre)
		{
		    if (act.x == hlf.x)
			oct = horpairs[oct];
		    else if (act.y == hlf.y)
			oct = verpairs[oct];
		    else if (act.z == hlf.z)
			oct = deppairs[oct];
		}

		pre = oct;

		int socti = oct_from_octets_for_index(oct, stck[level].socti, octtexbuf_s, level);
		int docti = oct_from_octets_for_index(oct, stck[level].docti, octtexbuf_d, level);

		// add to isps if there are subnodes in current cube
		if (socti > 0 || docti > 0)
		{
#ifdef OCTTEST
		    res.col.a += 0.2;
#endif
		    int ind               = stck[level].ispsi;
		    int len               = ind & 0x0F;
		    stck[level].octs[len] = oct;
		    stck[level].isps[len] = act;
		    len++;
		    stck[level].ispsi = (ind & 0xF0) | len;
		}
	    }
	}

	// go inside closest subnode

	int cur_len = stck[level].ispsi & 0x0F;

	if (cur_len > 0)
	{
	    int  nxt_ind = (stck[level].ispsi >> 4) & 7;
	    vec4 nxt_isp = stck[level].isps[nxt_ind];
	    int  nxt_oct = stck[level].octs[nxt_ind];

	    float halfs = tlf.w / 2.0;

	    tlf.x += xsft[nxt_oct] * halfs;
	    tlf.y -= ysft[nxt_oct] * halfs;
	    tlf.z -= zsft[nxt_oct] * halfs;
	    tlf.w = halfs;

	    nxt_ind++;
	    cur_len--;

	    // increase index and decrease length at current level
	    stck[level].ispsi = 128 | (nxt_ind << 4) | cur_len;

	    int socti = oct_from_octets_for_index(nxt_oct, stck[level].socti, octtexbuf_s, level);
	    int docti = oct_from_octets_for_index(nxt_oct, stck[level].docti, octtexbuf_d, level);

	    // increase stack level
	    level += 1;

	    // reset containers for the next level
	    stck[level].cube    = tlf;
	    stck[level].ispsi   = 0;
	    stck[level].socti   = socti;
	    stck[level].docti   = docti;
	    stck[level].isps[0] = nxt_isp;
	}
	else
	{
	    // no intersection with subnodes
	    // step back one for possible intersection with a neighbor node
	    stck[level--].ispsi = 0;

	    if (level < 0) return res;
	}
    }

    return res;
}

vec4 quat_from_axis_angle(vec3 axis, float angle)
{
    vec4  qr;
    float half_angle = angle * 0.5;
    qr.x             = axis.x * sin(half_angle);
    qr.y             = axis.y * sin(half_angle);
    qr.z             = axis.z * sin(half_angle);
    qr.w             = cos(half_angle);
    return qr;
}

vec3 quat_rotate(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}

const float PI = 3.1415926535897932384626433832795;

void main()
{
    // tan(60) = (dim.x / 2.0) / zdist -> zdist = (dim.x / 2.0) / tan(60)
    vec3 ctp = vec3(coord.xy, 0);                                                                  // camera target point
    vec3 cfp = vec3(dimensions.x / 2.0, dimensions.y / 2.0, (dimensions.x / 2.0) / tan(PI / 4.0)); // camera focus point
    vec3 csv = ctp - cfp;                                                                          // current screen vector

    vec4 qz = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), -angle_in.x); // rotation quaternion
    vec3 vx = quat_rotate(qz, vec3(-1.0, 0.0, 0.0));
    vec4 qx = quat_from_axis_angle(vx, -angle_in.y);

    // rotate and transform current screen vector based on camera vector

    csv = quat_rotate(qz, csv);
    csv = quat_rotate(qx, csv);

    // check angle between light-fp and cam vec, if small, check collosion

    vec3  camlight = light - camfp.xyz;
    float camangle = acos(dot(normalize(camlight), normalize(csv)));

    ctlres res = cube_trace_line(camfp, csv);
    vec4   col = res.col;

#ifndef OCTTEST
    if (res.isp.w > 0.0)
    {
	/* show normals for debug SWITCHABLE */
	/* col = vec4(abs(res.nrm.x), abs(res.nrm.y), abs(res.nrm.z), 1.0); */

	/* shadows, test against light SWITCHABLE */

	vec3   lghtv = res.isp.xyz - light;
	ctlres lcres = cube_trace_line(light, lghtv);
	vec3   ispv  = lcres.isp.xyz - res.isp.xyz;
	float  sqr   = ispv.x * ispv.x + ispv.y * ispv.y + ispv.z * ispv.z;

	float lght_nrm_ang = max(dot(normalize(-lghtv), normalize(res.nrm.xyz)), 0.0);
	float camv_nrm_ang = max(dot(normalize(-csv), normalize(res.nrm.xyz)), 0.0);

	// if isp distance is over 15.0, cube is shadow colored

	col = vec4(col.xyz * (0.1 + 0.2 * camv_nrm_ang + lght_nrm_ang * step(sqr, 15.0) * 0.7), col.w);

	/* yellowish color, SWITCHABLE */

	col.z *= 0.7;

	// gunshot light

	col.xyz += float(shoot) * camv_nrm_ang * 0.1;
    }

    if (camangle < 0.02)
    {
	vec3   lghtv = light - camfp.xyz;
	ctlres lcres = cube_trace_line(camfp, lghtv);
	vec3   resv  = lcres.isp.xyz - camfp.xyz;
	if (resv.x / lghtv.x > 1.0)
	    col = vec4(1.0, 1.0, 1.0, 1.0);
    }

#endif

    fragColor = col;
}
