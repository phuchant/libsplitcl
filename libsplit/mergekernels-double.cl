__kernel void merge21(__global double *b00,
__global double *b01,
__global double *b02,
const int n0)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
}
__kernel void merge22(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
}
__kernel void merge23(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
const int n2)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id];
}
}
__kernel void merge24(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
const int n3)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id];
}
}
__kernel void merge25(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
const int n4)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id];
}
}
__kernel void merge26(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
const int n5)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id];
}
}
__kernel void merge27(__global double *b00,
__global double *b01,
__global double *b02,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
const int n5,
__global double *b60,
__global double *b61,
__global double *b62,
const int n6)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id];
}
if (id < n6) {
b60[id] +=
b61[id] - b60[id] +
b62[id] - b60[id];
}
}
__kernel void merge31(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
}
__kernel void merge32(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
}
__kernel void merge33(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
const int n2)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id];
}
}
__kernel void merge34(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
const int n3)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id];
}
}
__kernel void merge35(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
const int n4)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id];
}
}
__kernel void merge36(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
__global double *b53,
const int n5)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id] +
b53[id] - b50[id];
}
}
__kernel void merge37(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
__global double *b53,
const int n5,
__global double *b60,
__global double *b61,
__global double *b62,
__global double *b63,
const int n6)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id] +
b53[id] - b50[id];
}
if (id < n6) {
b60[id] +=
b61[id] - b60[id] +
b62[id] - b60[id] +
b63[id] - b60[id];
}
}
__kernel void merge41(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
}
__kernel void merge42(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
}
__kernel void merge43(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
__global double *b24,
const int n2)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id] +
b24[id] - b20[id];
}
}
__kernel void merge44(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
__global double *b24,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
__global double *b34,
const int n3)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id] +
b24[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id] +
b34[id] - b30[id];
}
}
__kernel void merge45(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
__global double *b24,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
__global double *b34,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
__global double *b44,
const int n4)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id] +
b24[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id] +
b34[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id] +
b44[id] - b40[id];
}
}
__kernel void merge46(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
__global double *b24,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
__global double *b34,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
__global double *b44,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
__global double *b53,
__global double *b54,
const int n5)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id] +
b24[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id] +
b34[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id] +
b44[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id] +
b53[id] - b50[id] +
b54[id] - b50[id];
}
}
__kernel void merge47(__global double *b00,
__global double *b01,
__global double *b02,
__global double *b03,
__global double *b04,
const int n0,
__global double *b10,
__global double *b11,
__global double *b12,
__global double *b13,
__global double *b14,
const int n1,
__global double *b20,
__global double *b21,
__global double *b22,
__global double *b23,
__global double *b24,
const int n2,
__global double *b30,
__global double *b31,
__global double *b32,
__global double *b33,
__global double *b34,
const int n3,
__global double *b40,
__global double *b41,
__global double *b42,
__global double *b43,
__global double *b44,
const int n4,
__global double *b50,
__global double *b51,
__global double *b52,
__global double *b53,
__global double *b54,
const int n5,
__global double *b60,
__global double *b61,
__global double *b62,
__global double *b63,
__global double *b64,
const int n6)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id] +
b03[id] - b00[id] +
b04[id] - b00[id];
}
if (id < n1) {
b10[id] +=
b11[id] - b10[id] +
b12[id] - b10[id] +
b13[id] - b10[id] +
b14[id] - b10[id];
}
if (id < n2) {
b20[id] +=
b21[id] - b20[id] +
b22[id] - b20[id] +
b23[id] - b20[id] +
b24[id] - b20[id];
}
if (id < n3) {
b30[id] +=
b31[id] - b30[id] +
b32[id] - b30[id] +
b33[id] - b30[id] +
b34[id] - b30[id];
}
if (id < n4) {
b40[id] +=
b41[id] - b40[id] +
b42[id] - b40[id] +
b43[id] - b40[id] +
b44[id] - b40[id];
}
if (id < n5) {
b50[id] +=
b51[id] - b50[id] +
b52[id] - b50[id] +
b53[id] - b50[id] +
b54[id] - b50[id];
}
if (id < n6) {
b60[id] +=
b61[id] - b60[id] +
b62[id] - b60[id] +
b63[id] - b60[id] +
b64[id] - b60[id];
}
}
