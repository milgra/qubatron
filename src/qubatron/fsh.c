#version 300 es

precision highp float;

in vec2  coord;
out vec4 fragColor;

uniform vec3 camfp;
uniform vec3 angle_in;
uniform vec3 light;
uniform vec4 basecube;
uniform vec2 dimensions;

/* highp vec3 light = vec3(0.0, 2000.0, -500.0); // dynamic light */

// octets
//     4 5
//     6 7
// 0 1
// 2 3

const int horpairs[] = int[8](1, 0, 3, 2, 5, 4, 7, 6);
const int verpairs[] = int[8](2, 3, 0, 1, 6, 7, 4, 5);
const int deppairs[] = int[8](4, 5, 6, 7, 0, 1, 2, 3);

const float xsft[] = float[8](0.0, 1.0, 0.0, 1.0, 0.0, 1.0, 0.0, 1.0);
const float ysft[] = float[8](0.0, 0.0, 1.0, 1.0, 0.0, 0.0, 1.0, 1.0);
const float zsft[] = float[8](0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0);

const int nulloct[9] = int[9](0, 0, 0, 0, 0, 0, 0, 0, 0);

const float PI   = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

float       minc_size = 1.0;
const float maxc_size = 3.0;

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
    int  ind;
};

struct stck_t
{
    vec4 tlf;      // cube dimensions for stack level
    int  scube[9]; // static cube octets for stack level
    int  dcube[9]; // static cube octets for stack level

    bool checked;  // stack level is checked for intersection
    int  octs[4];  // octets for ispts
    vec4 ispts[4]; // is point for stack level cube
    int  ispt_len; // ispt arr length
    int  ispt_ind; // ispt arr index
};

// x - x coordinate of plane, lp - line pointm lv - line vector

vec4 is_cube_xplane(float x, vec3 lp, vec3 lv)
{
    vec4 r = vec4(0.0);
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
    vec4 r = vec4(0.0);
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
    vec4 r = vec4(0.0);
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

int[9] soctets_for_index(int i)
{
    i *= 3; // octet is 12 byte long in octet texture

    int cy = i / 8192;
    int cx = i - cy * 8192;

    ivec4 p0, p1, p2;

    /* ivec2 ts = textureSize(octtexbuf_s, 0); */
    /* if (cx <= ts.x && cy <= ts.y) */
    /* { */
    p0 = texelFetch(octtexbuf_s, ivec2(cx, cy), 0);
    p1 = texelFetch(octtexbuf_s, ivec2(cx + 1, cy), 0);
    p2 = texelFetch(octtexbuf_s, ivec2(cx + 2, cy), 0);

    return int[9](p0.x, p0.y, p0.z, p0.w, p1.x, p1.y, p1.z, p1.w, p2.x);
}

int[9] doctets_for_index(int i)
{
    i *= 3; // octet is 12 byte long in octet texture

    int cy = i / 8192;
    int cx = i - cy * 8192;

    ivec4 p0, p1, p2;

    /* ivec2 ts = textureSize(octtexbuf_d, 0); */
    /* if (cx <= ts.x && cy <= ts.y) */
    /* { */
    p0 = texelFetch(octtexbuf_d, ivec2(cx, cy), 0);
    p1 = texelFetch(octtexbuf_d, ivec2(cx + 1, cy), 0);
    p2 = texelFetch(octtexbuf_d, ivec2(cx + 2, cy), 0);

    return int[9](p0.x, p0.y, p0.z, p0.w, p1.x, p1.y, p1.z, p1.w, p2.x);
}

ctlres
cube_trace_line(vec3 pos, vec3 dir)
{
    int depth = 0; // current octree deptu

    int scb[9] = soctets_for_index(0);
    int dcb[9] = doctets_for_index(0);

    ctlres res;
    res.isp = vec4(0.0, 0.0, 0.0, 0.0);
    res.col = vec4(0.0, 0.5, 0.0, 0.1);
    res.ind = 0;

    stck_t stck[20];
    stck[0].scube    = scb;
    stck[0].dcube    = dcb;
    stck[0].checked  = false;
    stck[0].ispt_len = 0;
    stck[0].ispt_ind = 0;
    stck[0].tlf      = basecube;

    int btypestate = 0;

    while (true)
    {
	int scube[9] = stck[depth].scube;
	int dcube[9] = stck[depth].dcube;

	vec4 tlf = stck[depth].tlf;

	if (!stck[depth].checked)
	{
	    stck[depth].checked = true;

	    // get is points

	    int  hitc = 0; // hit count
	    vec4 hitp[5];  // hit point

	    vec4 act;
	    vec4 brb = vec4(tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0);

	    // NOTE : if-ing out or ordering is pairs are slower than this

	    // front side
	    act = is_cube_zplane(tlf.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		hitp[hitc++] = act;

	    // back side
	    act = is_cube_zplane(brb.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		hitp[hitc++] = act;

	    // left side
	    act = is_cube_xplane(tlf.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++] = act;

	    // right side
	    act = is_cube_xplane(brb.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++] = act;

	    // top side
	    act = is_cube_yplane(tlf.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++] = act;

	    // bottom side
	    act = is_cube_yplane(brb.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++] = act;

	    // there is intersection
	    if (hitc > 0)
	    {
		// inside an octet, set focus point as first isp
		if (hitc == 1) hitp[0] = vec4(pos, 0.0);

		// order side hitpoints if there are two intersections
		if (hitc > 1 && hitp[1].w < hitp[0].w) hitp[0] = hitp[1];

		// we can ignore the second isp from now
		hitc = 1;

		// check division planes
		vec4 hlf = brb + (tlf - brb) * 0.5;

		// z div plane
		act = is_cube_zplane(hlf.z, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		    hitp[hitc++] = act;

		// x div plane
		act = is_cube_xplane(hlf.x, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		    hitp[hitc++] = act;

		// y div plane
		act = is_cube_yplane(hlf.y, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		    hitp[hitc++] = act;

		// sort hitpoints

		vec4 act_hitp;
		int  oct     = 0;
		int  prevoct = -1;

		for (int i = 0; i < hitc; ++i)
		{
		    if (i < hitc - 1)
		    {
			for (int j = i + 1; j < hitc; ++j)
			{
			    if (hitp[j].w < hitp[i].w)
			    {
				act_hitp = hitp[i];
				hitp[i]  = hitp[j];
				hitp[j]  = act_hitp;
			    }
			}
		    }

		    // calculate octets for current smallest

		    act_hitp = hitp[i];

		    oct = 0;

		    if (act_hitp.x > hlf.x) oct = 1;
		    if (act_hitp.y < hlf.y) oct += 2;
		    if (act_hitp.z < hlf.z) oct += 4;

		    // from the second isp find octet pairs if needed
		    if (oct == prevoct)
		    {
			if (act_hitp.x == hlf.x)
			    oct = horpairs[oct];
			else if (act_hitp.y == hlf.y)
			    oct = verpairs[oct];
			else if (act_hitp.z == hlf.z)
			    oct = deppairs[oct];
		    }

		    prevoct = oct;

		    // add to ispts if there are subnodes in current cube
		    if (scube[oct] > 0 || dcube[oct] > 0)
		    {
			int len                = stck[depth].ispt_len;
			stck[depth].octs[len]  = oct;
			stck[depth].ispts[len] = act_hitp;
			stck[depth].ispt_len += 1;
		    }
		}
	    }
	}

	// go inside closest subnode

	if (stck[depth].ispt_len > 0)
	{
	    // show subdivisons
	    /* res.cl.w += 0.1; */
	    /* res.cl += nearest_isp.col * 0.1; */

	    int  nearest_ind = stck[depth].ispt_ind++;
	    vec4 nearest_isp = stck[depth].ispts[nearest_ind];
	    int  nearest_oct = stck[depth].octs[nearest_ind];

	    int near_dcube[9] = nulloct;
	    int near_scube[9] = nulloct;

	    if (scube[nearest_oct] > 0)
		near_scube = soctets_for_index(scube[nearest_oct]);

	    if (dcube[nearest_oct] > 0)
		near_dcube = doctets_for_index(dcube[nearest_oct]);

	    vec4 ntlf = vec4(
		tlf.x += xsft[nearest_oct] * tlf.w / 2.0,
		tlf.y -= ysft[nearest_oct] * tlf.w / 2.0,
		tlf.z -= zsft[nearest_oct] * tlf.w / 2.0,
		tlf.w / 2.0);

	    if (depth == 11)
	    {
		res.isp = nearest_isp;
		res.tlf = ntlf;

		res.ind = scube[8]; // original index is in the 8th node

		int cy = res.ind / 8192;
		int cx = res.ind - cy * 8192;

		res.col = texelFetch(coltexbuf_s, ivec2(cx, cy), 0);
		res.nrm = texelFetch(nrmtexbuf_s, ivec2(cx, cy), 0);

		if (dcube[nearest_oct] > 0)
		{
		    res.ind = dcube[8]; // original index is in the 8th node

		    cy = res.ind / 8192;
		    cx = res.ind - cy * 8192;

		    res.col = texelFetch(coltexbuf_d, ivec2(cx, cy), 0);
		    res.nrm = texelFetch(nrmtexbuf_d, ivec2(cx, cy), 0);
		}

		// add sub-detail fuzzyness SWITCHABLE
		if (nearest_isp.w < 1.0)
		    res.col.xyz += res.col.xyz * random(res.isp.xyz) * 0.4;

		return res;
	    }

	    stck[depth].ispt_len -= 1;

	    // increase stack depth
	    depth += 1;

	    // reset containers for the next depth
	    stck[depth].tlf      = ntlf;
	    stck[depth].ispt_len = 0;
	    stck[depth].ispt_ind = 0;
	    stck[depth].checked  = false;
	    stck[depth].scube    = near_scube;
	    stck[depth].dcube    = near_dcube;
	}
	else
	{
	    // no intersection with subnodes
	    // step back one for possible intersection with a neighbor node
	    stck[depth].checked = false;

	    depth -= 1;

	    if (depth < 0) return res;
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

// rotate vector
vec3 qrot(vec4 q, vec3 v)
{
    return v + 2.0 * cross(q.xyz, cross(q.xyz, v) + q.w * v);
}
// rotate vector (alternative)
/* vec3 qrot_2(vec4 q, vec3 v) */
/* { */
/*     return v * (q.w * q.w - dot(q.xyz, q.xyz)) + 2.0 * q.xyz * dot(q.xyz, v) + 2.0 * q.w * cross(q.xyz, v); */
/* } */
// combine quaternions
/* vec4 qmul(vec4 a, vec4 b) */
/* { */
/*     return vec4(cross(a.xyz, b.xyz) + a.xyz * b.w + b.xyz * a.w, a.w * b.w - dot(a.xyz, b.xyz)); */
/* } */
// inverse quaternion
/* vec4 qinv(vec4 q) */
/* { */
/*     return vec4(-q.xyz, q.w); */
/* } */
// perspective project
/* vec4 get_projection(vec3 v, vec4 pr) */
/* { */
/*     return vec4(v.xy * pr.xy, v.z * pr.z + pr.w, -v.z); */
/* } */
// transform by Spatial forward
/* vec3 trans_for(vec3 v, Spatial s) */
/* { */
/*     return qrot(s.rot, v * s.pos.w) + s.pos.xyz; */
/* } */
// transform by Spatial inverse
/* vec3 trans_inv(vec3 v, Spatial s) */
/* { */
/*     return qrot(vec4(-s.rot.xyz, s.rot.w), (v - s.pos.xyz) / s.pos.w); */
/* } */

void main()
{
    vec3 ctp = vec3(coord.xy, 500.0 - dimensions.y / 2.0);                // camera target point
    vec3 csv = ctp - vec3(dimensions.x / 2.0, dimensions.y / 2.0, 500.0); // current screen vector

    vec4 qz = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), -angle_in.x); // rotation quaternion
    vec3 vx = qrot(qz, vec3(-1.0, 0.0, 0.0));
    vec4 qx = quat_from_axis_angle(vx, -angle_in.y);

    // rotate and transform current screen vector based on camera vector

    csv = qrot(qz, csv);
    csv = qrot(qx, csv);

    // check angle between light-fp and cam vec, if small, check collosion

    vec3  camlight = light - camfp.xyz;
    float camangle = acos(dot(normalize(camlight), normalize(csv)));

    ctlres res        = cube_trace_line(camfp, csv);
    vec4   result_col = res.col;

    if (res.ind > 0)
    {
	vec4 result_nrm = res.nrm;
	result_col      = res.col;

	/* show normals for debug SWITCHABLE */
	/* fragColor = vec4(abs(result_nrm.x), abs(result_nrm.y), abs(result_nrm.z), 1.0); */

	/* shadows, test against light SWITCHABLE */
	vec3 lvec = res.isp.xyz - light;

	ctlres lcres = cube_trace_line(light, lvec); // light cube, cube touched by light
	if (lcres.isp.w > 0.0)
	{
	    // checking isp is needed in case of bigger cube sizes where light can touch the top
	    if (/* res.isp.x != lcres.isp.x && */
		/* res.isp.y != lcres.isp.y && */
		/* res.isp.z != lcres.isp.z && */
		res.tlf.x != lcres.tlf.x &&
		res.tlf.y != lcres.tlf.y &&
		res.tlf.z != lcres.tlf.z)
	    {
		result_col = vec4(result_col.xyz * 0.2, result_col.w);
	    }
	    else
	    {
		float angle = max(dot(normalize(-lvec), normalize(result_nrm.xyz)), 0.0);
		result_col  = vec4(result_col.xyz * (0.2 + angle * 0.8), result_col.w);
	    }
	}
    }

    fragColor = result_col;

    // dark yellowish look SWITCHABLE

    fragColor.xyz *= 0.8;
    fragColor.z *= 0.4;
    if (camangle < 0.02)
	fragColor = vec4(1.0, 1.0, 1.0, 1.0);
}
