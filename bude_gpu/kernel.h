const char *bude_kernel =
"/**\n"
" * BUDE OpenCL kernel file\n"
" **/\n"
"\n"
"// Numeric constants\n"
"#define ZERO\t\t0.0f\n"
"#define QUARTER 0.25f\n"
"#define HALF\t\t0.5f\n"
"#define ONE\t\t 1.0f\n"
"#define TWO\t\t 2.0f\n"
"#define FOUR\t\t4.0f\n"
"#define CNSTNT 45.0f\n"
"\n"
"#define HBTYPE_F 70\n"
"#define HBTYPE_E 69\n"
"\n"
"// The data structure for one atom - 16 bytes\n"
"#ifndef ATOM_STRUCT\n"
"#define ATOM_STRUCT\n"
"typedef struct _atom\n"
"{\n"
"\tfloat x,y,z;\n"
"\tint index;\n"
"} Atom;\n"
"\n"
"typedef struct\n"
"{\n"
"\tint\t hbtype;\n"
"\tfloat radius;\n"
"\tfloat hphb;\n"
"\tfloat elsc;\n"
"} FFParams;\n"
"\n"
"#define HARDNESS 38.0f\n"
"#define NPNPDIST\t5.5f\n"
"#define NPPDIST\t 1.0f\n"
"\n"
"#endif\n"
"\n"
"inline void compute_transformation_matrix(const float transform_0,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tconst float transform_1,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tconst float transform_2,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tconst float transform_3,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tconst float transform_4,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tconst float transform_5,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t__private float4 *restrict transform)\n"
"{\n"
"\tconst float sx = sin(transform_0);\n"
"\tconst float cx = cos(transform_0);\n"
"\tconst float sy = sin(transform_1);\n"
"\tconst float cy = cos(transform_1);\n"
"\tconst float sz = sin(transform_2);\n"
"\tconst float cz = cos(transform_2);\n"
"\n"
"\ttransform[0].x = cy*cz;\n"
"\ttransform[0].y = sx*sy*cz - cx*sz;\n"
"\ttransform[0].z = cx*sy*cz + sx*sz;\n"
"\ttransform[0].w = transform_3;\n"
"\ttransform[1].x = cy*sz;\n"
"\ttransform[1].y = sx*sy*sz + cx*cz;\n"
"\ttransform[1].z = cx*sy*sz - sx*cz;\n"
"\ttransform[1].w = transform_4;\n"
"\ttransform[2].x = -sy;\n"
"\ttransform[2].y = sx*cy;\n"
"\ttransform[2].z = cx*cy;\n"
"\ttransform[2].w = transform_5;\n"
"}\n"
"\n"
"__kernel void fasten_main(const int natlig,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst int natpro,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global Atom *restrict protein_molecule,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global Atom *restrict ligand_molecule,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_0,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_1,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_2,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_3,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_4,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global float *restrict transforms_5,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t__global float *restrict etotals,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst __global FFParams *restrict global_forcefield,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t__local\tFFParams *restrict forcefield,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst int num_atom_types,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\tconst int numTransforms)\n"
"{\n"
"\t// Get index of first TD\n"
"\tint ix = get_group_id(0)*get_local_size(0)*NUM_TD_PER_THREAD + get_local_id(0);\n"
"\n"
"\t// Have extra threads do the last member intead of return.\n"
"\t// A return would disable use of barriers, so not using return is better\n"
"\tix = ix < numTransforms ? ix : numTransforms - NUM_TD_PER_THREAD;\n"
"\n"
"\t// Copy forcefield parameter table to local memory\n"
"\tevent_t event = async_work_group_copy((__local float*)forcefield,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t(__global float*)global_forcefield,\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\tnum_atom_types*sizeof(FFParams)/sizeof(float),\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t0);\n"
"\n"
"\t// Compute transformation matrix to private memory\n"
"\tfloat etot[NUM_TD_PER_THREAD];\n"
"\tfloat4 transform[NUM_TD_PER_THREAD][3];\n"
"\tint lsz = get_local_size(0);\n"
"\tfor (int i = 0; i < NUM_TD_PER_THREAD; i++)\n"
"\t{\n"
"\t\tint index = ix + i*lsz;\n"
"\t\tcompute_transformation_matrix(transforms_0[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransforms_1[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransforms_2[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransforms_3[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransforms_4[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransforms_5[index],\n"
"\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\t\ttransform[i]);\n"
"\t\tetot[i] = ZERO;\n"
"\t}\n"
"\n"
"\t// Wait for forcefield copy to finish\n"
"\twait_group_events(1, &event);\n"
"\n"
"\t// Loop over ligand atoms\n"
"\tint il = 0;\n"
"\tdo\n"
"\t{\n"
"\t\t// Load ligand atom data\n"
"\t\tconst Atom l_atom = ligand_molecule[il];\n"
"\t\tconst FFParams l_params = forcefield[l_atom.index];\n"
"\t\tconst bool lhphb_ltz = l_params.hphb<ZERO;\n"
"\t\tconst bool lhphb_gtz = l_params.hphb>ZERO;\n"
"\n"
"\t\tfloat3 lpos[NUM_TD_PER_THREAD];\n"
"\t\tconst float4 linitpos = (float4)(l_atom.x,l_atom.y,l_atom.z,ONE);\n"
"\t\tfor (int i = 0; i < NUM_TD_PER_THREAD; i++)\n"
"\t\t{\n"
"\t\t\t// Transform ligand atom\n"
"\t\t\tlpos[i].x = transform[i][0].w + linitpos.x*transform[i][0].x + linitpos.y*transform[i][0].y + linitpos.z*transform[i][0].z;\n"
"\t\t\tlpos[i].y = transform[i][1].w + linitpos.x*transform[i][1].x + linitpos.y*transform[i][1].y + linitpos.z*transform[i][1].z;\n"
"\t\t\tlpos[i].z = transform[i][2].w + linitpos.x*transform[i][2].x + linitpos.y*transform[i][2].y + linitpos.z*transform[i][2].z;\n"
"\t\t}\n"
"\n"
"\t\t// Loop over protein atoms\n"
"\t\tint ip = 0;\n"
"\t\tdo\n"
"\t\t{\n"
"\t\t\t// Load protein atom data\n"
"\t\t\tconst Atom p_atom = protein_molecule[ip];\n"
"\t\t\tconst FFParams p_params = forcefield[p_atom.index];\n"
"\n"
"\t\t\tconst float radij\t = p_params.radius + l_params.radius;\n"
"\t\t\tconst float r_radij = native_recip(radij);\n"
"\n"
"\t\t\tconst float elcdst\t= (p_params.hbtype==HBTYPE_F && l_params.hbtype==HBTYPE_F) ? FOUR\t\t: TWO;\n"
"\t\t\tconst float elcdst1 = (p_params.hbtype==HBTYPE_F && l_params.hbtype==HBTYPE_F) ? QUARTER : HALF;\n"
"\t\t\tconst bool type_E\t = ((p_params.hbtype==HBTYPE_E || l_params.hbtype==HBTYPE_E));\n"
"\n"
"\t\t\tconst bool phphb_ltz = p_params.hphb<ZERO;\n"
"\t\t\tconst bool phphb_gtz = p_params.hphb>ZERO;\n"
"\t\t\tconst bool phphb_nz\t= p_params.hphb!=ZERO;\n"
"\t\t\tconst float p_hphb\t = p_params.hphb * (phphb_ltz && lhphb_gtz ? -ONE : ONE);\n"
"\t\t\tconst float l_hphb\t = l_params.hphb * (phphb_gtz && lhphb_ltz ? -ONE : ONE);\n"
"\t\t\tconst float distdslv = (phphb_ltz ? (lhphb_ltz ? NPNPDIST : NPPDIST) : (lhphb_ltz ? NPPDIST : -FLT_MAX));\n"
"\t\t\tconst float r_distdslv = native_recip(distdslv);\n"
"\n"
"\t\t\tconst float chrg_init = l_params.elsc * p_params.elsc;\n"
"\t\t\tconst float dslv_init = p_hphb + l_hphb;\n"
"\n"
"\t\t\tfor (int i = 0; i < NUM_TD_PER_THREAD; i++)\n"
"\t\t\t{\n"
"\t\t\t\t// Calculate distance between atoms\n"
"\t\t\t\tconst float x\t\t\t= lpos[i].x - p_atom.x;\n"
"\t\t\t\tconst float y\t\t\t= lpos[i].y - p_atom.y;\n"
"\t\t\t\tconst float z\t\t\t= lpos[i].z - p_atom.z;\n"
"\t\t\t\tconst float distij = native_sqrt(x*x + y*y + z*z);\n"
"\n"
"\t\t\t\t// Calculate the sum of the sphere radii\n"
"\t\t\t\tconst float distbb = distij - radij;\n"
"\t\t\t\tconst bool\tzone1\t= (distbb < ZERO);\n"
"\n"
"\t\t\t\t// Calculate steric energy\n"
"\t\t\t\tetot[i] += (ONE - (distij*r_radij)) * (zone1 ? 2*HARDNESS : ZERO);\n"
"\n"
"\t\t\t\t// Calculate formal and dipole charge interactions\n"
"\t\t\t\tfloat chrg_e = chrg_init * ((zone1 ? 1 : (ONE - distbb*elcdst1)) * (distbb<elcdst ? 1 : ZERO));\n"
"\t\t\t\tfloat neg_chrg_e = -fabs(chrg_e);\n"
"\t\t\t\tchrg_e = type_E ? neg_chrg_e : chrg_e;\n"
"\t\t\t\tetot[i] += chrg_e*CNSTNT;\n"
"\n"
"\t\t\t\t// Calculate the two cases for Nonpolar-Polar repulsive interactions\n"
"\t\t\t\tfloat coeff\t= (ONE - (distbb*r_distdslv));\n"
"\t\t\t\tfloat dslv_e = dslv_init * ((distbb<distdslv && phphb_nz) ? 1 : ZERO);\n"
"\t\t\t\tdslv_e *= (zone1 ? 1 : coeff);\n"
"\t\t\t\tetot[i] += dslv_e;\n"
"\t\t\t}\n"
"\t\t} while (++ip < natpro); // loop over protein atoms\n"
"\t} while (++il < natlig); // loop over ligand atoms\n"
"\n"
"\t// Write results\n"
"\tint td_base = get_group_id(0)*get_local_size(0)*NUM_TD_PER_THREAD + get_local_id(0);\n"
"\tif (td_base < numTransforms)\n"
"\t{\n"
"\t\tfor (int i = 0; i < NUM_TD_PER_THREAD; i++)\n"
"\t\t{\n"
"\t\t\tetotals[td_base+i*get_local_size(0)] = etot[i]*HALF;\n"
"\t\t}\n"
"\t}\n"
"} //end of fasten_main\n"
;
