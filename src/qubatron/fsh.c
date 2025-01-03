#version 310 es

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

struct octets_t
{
    // 8 octets
    // the 9th element is the original index of the point
    // we need 12 element for std430 padding
    int nodes[12];
};

const float PI   = 3.1415926535897932384626433832795;
const float PI_2 = 1.57079632679489661923;

float       minc_size = 1.0;
const float maxc_size = 3.0;

int  model_state = 0; // 0 static 1 dynamic
bool procgen = false;

layout(std430, binding = 1) readonly buffer octreelayout_s
{
    octets_t g_octree_s[];
};

layout(std430, binding = 2) readonly buffer normallayout_s
{
    vec4 g_normals_s[];
};

layout(std430, binding = 3) readonly buffer colorlayout_s
{
    vec4 g_colors_s[];
};

layout(std430, binding = 4) readonly buffer octreelayout_d
{
    octets_t g_octree_d[];
};

layout(std430, binding = 5) readonly buffer normallayout_d
{
    vec4 g_normals_d[];
};

layout(std430, binding = 6) readonly buffer colorlayout_d
{
    vec4 g_colors_d[];
};

struct ctlres
{
    vec4 isp;
    vec4 col;
    vec4 nrm;
    vec4 tlf;
    int  ind;
};

struct ispt_t // intersection struct
{
    vec4 isp; // intersection point
    /* vec4 col; // side color for debugging */
    int oct; // octet of isp
};

struct stck_t
{
    vec4     tlf;      // cube dimensions for stack level
    octets_t cube;     // cube normal, color and octets for stack level
    bool     checked;  // stack level is checked for intersection
    int      octs[4];  // octets for ispts
    ispt_t   ispts[4]; // is point for stack level cube
    int      ispt_len; // ispt length for is points
    int      ispt_ind;
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
	r.w = (z - lp.z) / lv.z; // distance ratio
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

ctlres
cube_trace_line(vec3 pos, vec3 dir)
{
    int depth = 0; // current octree deptu

    octets_t cb;
    if (model_state == 0)
	cb = g_octree_s[0];
    else
	cb = g_octree_d[0];

    ctlres res;
    res.isp = vec4(0.0, 0.0, 0.0, 0.0);
    res.col = vec4(0.0, 0.5, 0.0, 0.1);
    res.ind = 0;

    stck_t stck[20];
    stck[0].cube     = cb;
    stck[0].checked  = false;
    stck[0].ispt_len = 0;
    stck[0].ispt_ind = 0;
    stck[0].tlf      = basecube;

    octets_t ccube;

    while (true)
    {
	ccube    = stck[depth].cube;
	vec4 tlf = stck[depth].tlf;

	if (!stck[depth].checked)
	{
	    stck[depth].checked = true;

	    // get is points

	    int    hitc = 0; // hit count
	    ispt_t hitp[5];  // hit point

	    vec4 act;
	    vec4 brb = vec4(tlf.x + tlf.w, tlf.y - tlf.w, tlf.z - tlf.w, 0.0);

	    // NOTE : if-ing out or ordering is pairs are slower than this

	    // front side
	    act = is_cube_zplane(tlf.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		hitp[hitc++].isp = act;

	    // back side
	    act = is_cube_zplane(brb.z, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		hitp[hitc++].isp = act;

	    // left side
	    act = is_cube_xplane(tlf.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++].isp = act;

	    // right side
	    act = is_cube_xplane(brb.x, pos, dir);
	    if (act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++].isp = act;

	    // top side
	    act = is_cube_yplane(tlf.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++].isp = act;

	    // bottom side
	    act = is_cube_yplane(brb.y, pos, dir);
	    if (act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		hitp[hitc++].isp = act;

	    // there is intersection
	    if (hitc > 0)
	    {
		// inside an octet, set focus point as first isp
		if (hitc == 1) hitp[0].isp = vec4(pos, 0.0);

		// order side hitpoints if there are two intersections
		if (hitc > 1 && hitp[1].isp.w < hitp[0].isp.w) hitp[0] = hitp[1];

		// we can ignore the second isp from now
		hitc = 1;

		// check division planes
		vec4 hlf = brb + (tlf - brb) * 0.5;

		// z div plane
		act = is_cube_zplane(hlf.z, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.y > act.y && act.y >= brb.y)
		    hitp[hitc++].isp = act;

		// x div plane
		act = is_cube_xplane(hlf.x, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.y > act.y && act.y >= brb.y && tlf.z > act.z && act.z >= brb.z)
		    hitp[hitc++].isp = act;

		// y div plane
		act = is_cube_yplane(hlf.y, pos, dir);
		if (act.w > 0.0 && act.w > 0.0 && tlf.x < act.x && act.x <= brb.x && tlf.z > act.z && act.z >= brb.z)
		    hitp[hitc++].isp = act;

		// sort hitpoints

		ispt_t act_hitp;
		int    oct     = 0;
		int    prevoct = -1;

		for (int i = 0; i < hitc; ++i)
		{
		    if (i < hitc - 1)
		    {
			for (int j = i + 1; j < hitc; ++j)
			{
			    if (hitp[j].isp.w < hitp[i].isp.w)
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

		    if (act_hitp.isp.x > hlf.x) oct = 1;
		    if (act_hitp.isp.y < hlf.y) oct += 2;
		    if (act_hitp.isp.z < hlf.z) oct += 4;

		    // from the second isp find octet pairs if needed
		    if (oct == prevoct)
		    {
			if (act_hitp.isp.x == hlf.x)
			    oct = horpairs[oct];
			else if (act_hitp.isp.y == hlf.y)
			    oct = verpairs[oct];
			else if (act_hitp.isp.z == hlf.z)
			    oct = deppairs[oct];
		    }

		    prevoct = oct;

		    // add to ispts if there are subnodes in current cube
		    if (ccube.nodes[oct] > 0)
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

	    int      nearest_ind = stck[depth].ispt_ind++;
	    ispt_t   nearest_isp = stck[depth].ispts[nearest_ind];
	    int      nearest_oct = stck[depth].octs[nearest_ind];
	    octets_t nearest_cube;

	    vec4 ntlf = vec4(
		tlf.x += xsft[nearest_oct] * tlf.w / 2.0,
		tlf.y -= ysft[nearest_oct] * tlf.w / 2.0,
		tlf.z -= zsft[nearest_oct] * tlf.w / 2.0,
		tlf.w / 2.0);

	    if (!procgen)
	    {
		if (model_state == 0)
		    nearest_cube = g_octree_s[ccube.nodes[nearest_oct]];
		else if (model_state == 1)
		    nearest_cube = g_octree_d[ccube.nodes[nearest_oct]];
	    }
	    else
	    {
		// generate subnode proced

		/* nearest_cube       = octets_t(ccube.tlf, ccube.nrm, ccube.col, int[8](1, 1, 1, 1, 1, 1, 1, 1)); */
		/* nearest_cube.tlf.w = ccube.tlf.w / 2.0; */

		/* nearest_cube.tlf.x += xsft[nearest_oct] * nearest_cube.tlf.w; */
		/* nearest_cube.tlf.y -= ysft[nearest_oct] * nearest_cube.tlf.w; */
		/* nearest_cube.tlf.z -= zsft[nearest_oct] * nearest_cube.tlf.w; */

		/* float rnd = -0.05 + random(nearest_cube.tlf.xyz) / 10.0; */

		/* nearest_cube.col -= nearest_cube.col * rnd; */

		/* // fill up octets based on nearby octets */

		/* int pick                 = int(round(random(nearest_cube.tlf.xyz) * 8.0)); */
		/* nearest_cube.nodes[pick] = 0; */
	    };

	    if (depth == 11)
	    {
		/* float visw = nearest_cube.tlf.w / nearest_isp.isp.w; */
		/* if (visw > maxc_size) */
		/* { */
		/*     procgen = true; */
		/*     /\* minc_size = 0.5; *\/ */
		/*     nearest_cube.nodes       = int[8](1, 1, 1, 1, 1, 1, 1, 1); */
		/*     int pick                 = int(round(random(nearest_cube.tlf.xyz) * 7.0)); */
		/*     nearest_cube.nodes[pick] = 0; */
		/*     /\* res.isp                  = nearest_isp.isp; *\/ */
		/*     /\* res.col                  = nearest_cube.col; *\/ */
		/*     /\* res.nrm                  = nearest_cube.nrm; *\/ */
		/*     /\* res.tlf                  = nearest_cube.tlf; *\/ */
		/*     /\* res.col                  = nearest_isp.col; *\/ */
		/*     /\* res.col = vec4(1.0, 0.0, 0.0, 1.0); *\/ */
		/*     /\* return res; *\/ */
		/* } */
		/* else */
		/* { */
		res.isp = nearest_isp.isp;
		res.tlf = ntlf;
		res.ind = nearest_cube.nodes[8]; // original index is in the 8th node
		/* res.col = nearest_isp.col; */
		return res;
		/* } */
	    }

	    stck[depth].ispt_len -= 1;

	    // increase stack depth
	    depth += 1;

	    // reset containers for the next depth
	    stck[depth].tlf      = ntlf;
	    stck[depth].ispt_len = 0;
	    stck[depth].ispt_ind = 0;
	    stck[depth].cube     = nearest_cube;
	    stck[depth].checked  = false;

	    /* makes sense in case of procedural sub-detail render */
	    /* if (depth > 12) */
	    /* { */
	    /* 	res.isp = nearest_isp.isp; */
	    /* 	res.tlf = ntlf; */
	    /* 	res.ind = nearest_cube.nodes[8]; */
	    /* 	/\* res.col = vec4(1.0, 0.0, 0.0, 1.0); *\/ */
	    /* 	return res; */
	    /* } */
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
    /* vec3 csv = ctp - vec3(300.0, 200.0, 500.0);                           // current screen vector */

    vec4 qz = quat_from_axis_angle(vec3(0.0, 1.0, 0.0), -angle_in.x); // rotation quaternion
    vec3 vx = qrot(qz, vec3(-1.0, 0.0, 0.0));
    vec4 qx = quat_from_axis_angle(vx, -angle_in.y);

    csv = qrot(qz, csv);
    csv = qrot(qx, csv);

    // rotate and transform current screen vector based on camera vector

    /* x' = x cos θ − y sin θ */
    /* y' = x sin θ + y cos θ */

    /* float rad = -0.6; */
    /* float nx  = csv.x * cos(rad) - csv.z * sin(rad); */
    /* float nz  = csv.x * sin(rad) + csv.z * cos(rad); */

    /* csv.x = nx; */
    /* csv.z = nz; */

    /* cam_fp.z -= 300.0; */

    // octets_t cb0 = cube(vec3(300.0, 300.0, 0.0), vec3(600.0, 600.0, -300.0), 300.0, 8);
    /* octets_t cb0 = g_octree_s[0]; */

    /* vec4 col = vec4(0.0, 0.0, 0.0, 0.0); */
    /* for (int i = 0; i < 10; i++) */
    /* { */
    /* 	vec4 ncol = octets_trace_line_sep(g_octree_s[i], cam_fp, csv); */
    /* 	col             = col + ncol; */
    /* } */

    // check angle between light-fp and cam vec, if small, check collosion

    vec3  camlight = light - camfp.xyz;
    float camangle = acos(dot(normalize(camlight), normalize(csv)));
    if (camangle < 0.02)
    {
	fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }
    else
    {
	vec4   col;
	ctlres ccres_s = cube_trace_line(camfp, csv); // camera cube, cube touched by cam
	model_state    = 1;                           // have to do this for cube_trace_line function's global ssbo access
	ctlres ccres_d = cube_trace_line(camfp, csv); // camera cube, cube touched by cam

	// TODO ssbo switching is fucked up, do something

	int  result_state = 0;
	vec4 result_isp   = ccres_s.isp;
	vec4 result_tlf   = ccres_s.tlf;
	vec4 result_col   = g_colors_s[ccres_s.ind];
	vec4 result_nrm   = g_normals_s[ccres_s.ind];

	if (ccres_d.ind > 0 && (ccres_s.ind == 0 || ccres_d.isp.w < ccres_s.isp.w))
	{
	    result_state = 1;
	    result_isp   = ccres_d.isp;
	    result_tlf   = ccres_d.tlf;
	    result_col   = g_colors_d[ccres_d.ind];
	    result_nrm   = g_normals_d[ccres_d.ind];
	}

	/* if (ccres_d.ind == 0 && ccres_s.ind == 0) result_col = ccres_s.col; */

	fragColor = result_col;

	/* show normals */
	/* fragColor = vec4(abs(ccres.nrm.x), abs(ccres.nrm.y), abs(ccres.nrm.z), 1.0); */

	// if we found cube, get normal and color from corresponding arrays
	// else show debug color ( put intersection count in ccres?

	// we found a subcube
	if (result_isp.w > 0.0)
	{
	    /* test against light */
	    vec3 lvec = result_isp.xyz - light;

	    model_state  = result_state;
	    ctlres lcres = cube_trace_line(light, lvec); // light cube, cube touched by light
	    if (lcres.isp.w > 0.0)
	    {
		if (result_isp.x != lcres.isp.x &&
		    result_isp.y != lcres.isp.y &&
		    result_isp.z != lcres.isp.z &&
		    result_tlf.x != lcres.tlf.x &&
		    result_tlf.y != lcres.tlf.y &&
		    result_tlf.z != lcres.tlf.z)
		/* if ((abs(result_isp.x - lcres.isp.x) < 0.01) && */
		/* 	(abs(result_isp.y - lcres.isp.y) < 0.01) && */
		/* 	(abs(result_isp.z - lcres.isp.z) < 0.01)) */
		{
		    fragColor = vec4(fragColor.xyz * 0.2, fragColor.w);
		}
		else
		{
		    float angle = max(dot(normalize(-lvec), normalize(result_nrm.xyz)), 0.0);
		    fragColor   = vec4(fragColor.xyz * (0.2 + angle * 0.8), fragColor.w);
		}
	    }
	}

	// dark yellowish look
	fragColor *= 0.8;
	fragColor.z *= 0.4;
    }
};
