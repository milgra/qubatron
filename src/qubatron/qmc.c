// qubatron model converter

#include <getopt.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "mt_string.c"
#include "mt_vector_3d.c"
#include "rply.h"

float minpx, maxpx, minpy, maxpy, minpz, maxpz;
float px, py, pz, cx, cy, cz, nx, ny, nz;

int offx        = 0;
int offy        = 0;
int offz        = 0;
int cnt         = 0;
int point_count = 0;
int ind         = 0;

int dropped_count = 0;

int division = 2;

float precision;

typedef struct _point_t
{
    v3_t point;
    v3_t color;
    v3_t normal;
    v3_t index;
} point_t;

size_t   pointi = 0;
point_t* points = NULL;

static int
model_vertex_cb(p_ply_argument argument)
{
    long eol;

    ply_get_argument_user_data(argument, NULL, &eol);

    if (ind == 0) px = (ply_get_argument_value(argument)) - (float) offx;
    if (ind == 1) py = (ply_get_argument_value(argument)) - (float) offy;
    if (ind == 2) pz = (ply_get_argument_value(argument)) - (float) offz;

    if (ind == 3) cx = ply_get_argument_value(argument) / 255.0;
    if (ind == 4) cy = ply_get_argument_value(argument) / 255.0;
    if (ind == 5) cz = ply_get_argument_value(argument) / 255.0;

    if (ind == 6) nx = ply_get_argument_value(argument);
    if (ind == 7) ny = ply_get_argument_value(argument);
    if (ind == 8)
    {
	nz = ply_get_argument_value(argument);

	float xi = floor(px / precision);
	float yi = floor(py / precision);
	float zi = floor(pz / precision);

	if (xi >= 0.0 && xi < (float) division &&
	    yi >= 0.0 && yi < (float) division &&
	    zi >= 0.0 && zi < (float) division)
	{
	    xi = floor(px / precision);
	    yi = floor(py / precision);
	    zi = floor(pz / precision);

	    point_t point = {
		.point  = (v3_t){px, py, pz},
		.color  = (v3_t){cx, cy, cz},
		.normal = (v3_t){nx, ny, nz},
		.index  = (v3_t){xi, yi, zi}};

	    /* printf("cx %f cy %f cz %f xi %f yi %f zi %f\n", cx, cy, cz, point.index.x, point.index.y, point.index.z); */

	    points[pointi++] = point;
	}
	else
	{
	    printf("dropping xi %f yi %f zi %f division %f\n", xi, yi, zi, (float) division);
	    dropped_count++;
	}

	if (cnt == 0)
	{
	    minpx = px;
	    minpy = py;
	    minpz = pz;
	    maxpx = px;
	    maxpy = py;
	    maxpz = pz;
	}
	else
	{
	    if (px < minpx) minpx = px;
	    if (py < minpy) minpy = py;
	    if (pz < minpz) minpz = pz;
	    if (px > maxpx) maxpx = px;
	    if (py > maxpy) maxpy = py;
	    if (pz > maxpz) maxpz = pz;
	}

	if (cnt % 100000 == 0) printf("parsing %f\n\033[1A", (float) cnt / (float) point_count);

	cnt++;
	ind = -1;
    }

    ind++;

    /* if (cnt == 10000) return 0; */
    /* if (cnt < 1000) */
    /* { */
    /* 	printf("%li : %g", ind, ply_get_argument_value(argument)); */
    /* 	if (eol) */
    /* 	    printf("\n"); */
    /* 	else */
    /* 	    printf(" "); */
    /* } */
    return 1;
}

int compi = 0;
int comp(const void* elem1, const void* elem2)
{
    if (compi++ % 100000 == 0) printf("sorting %f\n\033[1A", (float) compi / point_count * 4.3);

    point_t* pa = (point_t*) elem1;
    point_t* pb = (point_t*) elem2;

    if (pa->index.x == pb->index.x)
    {
	if (pa->index.y == pb->index.y)
	{
	    if (pa->index.z == pb->index.z)
		return 0;
	    else if (pa->index.z < pb->index.z)
		return -1;
	    else
		return 1;
	}
	else if (pa->index.y < pb->index.y)
	{
	    return -1;
	}
	else
	    return 1;
    }
    else if (pa->index.x < pb->index.x)
    {
	return -1;
    }
    else
    {
	return 1;
    }

    return 0;
}

int main(int argc, char* argv[])
{
    printf("Qubatron Model Converter v" QMC_VERSION " by Milan Toth ( www.milgra.com )\n");

    int   size   = 1000;
    int   levels = 10;
    char* input;

    const char* usage =
	"Usage: qmv [options]\n"
	"\n"
	"  -h, --help                       Show help message and quit.\n"
	"  -s, --size                       Scene cube size in coordinate system, default is 1000\n"
	"  -l, --levels                     Division level, default is 10\n"
	"  -i, --input                      Input file, ply with vertex, color and normal data\n"
	"  -x, --offsetx                    x offset, default 0\n"
	"  -y, --offsety                    y offset, default 0\n"
	"  -z, --offsetz                    z offsetm default 0\n"
	"\n";

    const struct option long_options[] = {
	{"help", no_argument, NULL, 'h'},
	{"size", no_argument, NULL, 's'},
	{"levels", no_argument, NULL, 'l'},
	{"input", no_argument, NULL, 'i'},
	{"offsetx", no_argument, NULL, 'x'},
	{"offsety", no_argument, NULL, 'y'},
	{"offsetz", no_argument, NULL, 'z'}};

    int option       = 0;
    int option_index = 0;

    while ((option = getopt_long(argc, argv, "hs:l:i:x:y:z:", long_options, &option_index)) != -1)
    {
	switch (option)
	{
	    case '?': printf("parsing option %c value: %s\n", option, optarg); break;
	    case 's': size = atoi(optarg); break;
	    case 'l': levels = atoi(optarg); break;
	    case 'i':
		input = mt_string_new_cstring(optarg);
		break;
	    case 'x': offx = atoi(optarg); break;
	    case 'y': offy = atoi(optarg); break;
	    case 'z': offz = atoi(optarg); break;

	    default: fprintf(stderr, "%s", usage); return EXIT_FAILURE;
	}
    }

    for (int i = 0; i < levels; i++) division *= 2;
    precision = (float) size / (float) division;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) printf("Cannot get working directory\n");

    char plypath[PATH_MAX];
    snprintf(plypath, PATH_MAX, "%s/%s", cwd, input);

    printf("size %i\nlevels %i\ndivisions %i\nprecision %f\ninput path %s\noffx %i\noffy %i\noffz %i\n", size, levels, division, precision, plypath, offx, offy, offz);

    p_ply ply = ply_open(plypath, NULL, 0, NULL);

    if (!ply) printf("cannot read %s\n", plypath);
    if (!ply_read_header(ply)) printf("cannot read ply header\n");

    point_count = ply_set_read_cb(ply, "vertex", "x", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "y", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "z", model_vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "red", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "green", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "blue", model_vertex_cb, NULL, 1);
    ply_set_read_cb(ply, "vertex", "nx", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "ny", model_vertex_cb, NULL, 0);
    ply_set_read_cb(ply, "vertex", "nz", model_vertex_cb, NULL, 1);

    printf("ply point count : %u\n", point_count);

    points = mt_memory_alloc(sizeof(point_t) * point_count, NULL, NULL);

    if (!ply_read(ply)) printf("PLY read error\n");
    ply_close(ply);

    printf(
	"minpx %f\nmaxpx %f\nminpy %f\nmaxpy %f\nminpz %f\nmaxpz %f\n",
	minpx, maxpx,
	minpy, maxpy,
	minpz, maxpz);

    printf("points count %li out of bounds point dropped %i\n", pointi, dropped_count);

    printf("sorting points\n");
    qsort(points, pointi, sizeof(point_t), comp);

    printf("dropping same position points\n");

    size_t  npi  = 0;
    point_t prep = {0};

    FILE* pntfile;
    FILE* nrmfile;
    FILE* colfile;
    FILE* rngfile;

    char pntpath[PATH_MAX];
    char nrmpath[PATH_MAX];
    char colpath[PATH_MAX];
    char rngpath[PATH_MAX];
    snprintf(pntpath, PATH_MAX, "%s/%s.pnt", cwd, input);
    snprintf(nrmpath, PATH_MAX, "%s/%s.nrm", cwd, input);
    snprintf(colpath, PATH_MAX, "%s/%s.col", cwd, input);
    snprintf(rngpath, PATH_MAX, "%s/%s.rng", cwd, input);

    pntfile = fopen(pntpath, "wb");
    nrmfile = fopen(nrmpath, "wb");
    colfile = fopen(colpath, "wb");
    rngfile = fopen(rngpath, "wb");

    if (!pntfile || !nrmfile || !colfile || !rngfile)
    {
	printf("Unable to open file!\n");
	return 1;
    }

    for (int i = 0; i < pointi; i++)
    {
	if (i % 1000 == 0) printf("dropping %f\n\033[1A", (float) i / (float) pointi);

	point_t actp = points[i];

	if (!(i > 0 && prep.index.x == actp.index.x && prep.index.y == actp.index.y && prep.index.z == actp.index.z))
	{
	    if (prep.index.x != actp.index.x || prep.index.y != actp.index.y)
	    {
		int ix = (int) actp.index.x;
		int iy = (int) actp.index.y;

		// write x y range

		fwrite(&npi, sizeof(int), 1, rngfile);
		fwrite(&ix, sizeof(int), 1, rngfile);
		fwrite(&iy, sizeof(int), 1, rngfile);
	    }

	    // write position normal and color

	    fwrite(&actp.point.x, sizeof(float), 1, pntfile);
	    fwrite(&actp.point.y, sizeof(float), 1, pntfile);
	    fwrite(&actp.point.z, sizeof(float), 1, pntfile);
	    fwrite(&actp.normal.x, sizeof(float), 1, nrmfile);
	    fwrite(&actp.normal.y, sizeof(float), 1, nrmfile);
	    fwrite(&actp.normal.z, sizeof(float), 1, nrmfile);
	    fwrite(&actp.color.x, sizeof(float), 1, colfile);
	    fwrite(&actp.color.y, sizeof(float), 1, colfile);
	    fwrite(&actp.color.z, sizeof(float), 1, colfile);

	    npi++;
	}

	prep = actp;
    }

    fclose(pntfile);
    fclose(nrmfile);
    fclose(colfile);
    fclose(rngfile);

    printf("compacted point count %li dropped count %li ratio %f\n", npi, pointi - npi, (float) npi / (float) pointi);

    return 0;
}
