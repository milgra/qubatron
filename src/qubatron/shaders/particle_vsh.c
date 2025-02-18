#version 300 es

precision highp float;

in vec3 pos;
in vec3 spd;

out vec3 pos_out;
out vec3 spd_out;

uniform vec4 basecube;
uniform int  maxlevel;

uniform sampler2D coltexbuf_s; // color data per point for static model
uniform sampler2D coltexbuf_d; // color data per point for dynamic model

uniform sampler2D nrmtexbuf_s; // normal data per point for static
uniform sampler2D nrmtexbuf_d; // normal data per point for dynamic

uniform lowp isampler2D octtexbuf_s; // octree for static in a format, nine ints, first 8 is octets for cube, 9th is index for next octet/color/normal
uniform lowp isampler2D octtexbuf_d; // octree for dynamic

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

const float PI   = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

// A line point 0
// B line point 1
// C point to project

vec3 project_point(vec3 A, vec3 B, vec3 C)
{
    vec3 AC = C - A;
    vec3 AB = B - A;
    return A + dot(AC, AB) / dot(AB, AB) * AB;
}

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

// !!! TODO octet texture and index texture buffer should be separate to save size and bandwidth

int oct_from_octets_for_index(int octi, int i, lowp isampler2D sampler, int level)
{
    if (i == 0 && level > 0) return 0;
    i *= 3; // octet is 12 byte long in octet texture

    int cy = i / 8192;
    int cx = i - cy * 8192 + octi / 4;

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
    if (hitc < 2) return res;

    // one hitp have to be in front of the camera
    if (hitp[0].w < 0.0 && hitp[1].w < 0.0) return res;

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

void main()
{
    // add gravity to speed, slow down to simulate friction

    vec3 vspd = spd;

    vspd += vec3(0.0, -1.0, 0.0);
    vspd *= 0.9;

    // check collosion with actual step

    pos_out = vec3(0.0, 0.0, 0.0);
    spd_out = vec3(0.0, 0.0, 0.0);

    if (length(vspd) < 1.0) return;

    pos_out = pos + vspd;
    spd_out = vspd;

    /* ctlres res = cube_trace_line(pos, vspd); */

    /* if (res.isp.w == 0.0) return; */

    /* // if speed is big and particle is solid, bounce using normal of surface */

    /* vec3 projp = project_point(res.isp.xyz, res.isp.xyz + res.nrm.xyz, res.isp.xyz - vspd); */
    /* vec3 mirrp = projp + projp - (res.isp.xyz - vspd); */
    /* vspd       = mirrp - res.isp.xyz; */

    /* pos_out = projp; */
    /* spd_out = vspd; */

    /* return; */

    // if speed is slow or not solid particle, stall, particle will be added to static model

    // this shader should calculate the octree path for speedup like in skeleton_vsh
}
