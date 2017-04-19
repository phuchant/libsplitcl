__kernel void merge21(__global float *b00,
__global float *b01,
__global float *b02,
const int n0)
{
int id = get_global_id(0);
if (id < n0) {
b00[id] +=
b01[id] - b00[id] +
b02[id] - b00[id];
}
}
__kernel void merge22(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
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
__kernel void merge23(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
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
__kernel void merge24(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
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
__kernel void merge25(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
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
__kernel void merge26(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
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
__kernel void merge27(__global float *b00,
__global float *b01,
__global float *b02,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
const int n5,
__global float *b60,
__global float *b61,
__global float *b62,
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
__kernel void merge31(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
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
__kernel void merge32(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
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
__kernel void merge33(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
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
__kernel void merge34(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
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
__kernel void merge35(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
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
__kernel void merge36(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
__global float *b53,
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
__kernel void merge37(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
__global float *b53,
const int n5,
__global float *b60,
__global float *b61,
__global float *b62,
__global float *b63,
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
__kernel void merge41(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
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
__kernel void merge42(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
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
__kernel void merge43(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
__global float *b24,
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
__kernel void merge44(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
__global float *b24,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
__global float *b34,
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
__kernel void merge45(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
__global float *b24,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
__global float *b34,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
__global float *b44,
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
__kernel void merge46(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
__global float *b24,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
__global float *b34,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
__global float *b44,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
__global float *b53,
__global float *b54,
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
__kernel void merge47(__global float *b00,
__global float *b01,
__global float *b02,
__global float *b03,
__global float *b04,
const int n0,
__global float *b10,
__global float *b11,
__global float *b12,
__global float *b13,
__global float *b14,
const int n1,
__global float *b20,
__global float *b21,
__global float *b22,
__global float *b23,
__global float *b24,
const int n2,
__global float *b30,
__global float *b31,
__global float *b32,
__global float *b33,
__global float *b34,
const int n3,
__global float *b40,
__global float *b41,
__global float *b42,
__global float *b43,
__global float *b44,
const int n4,
__global float *b50,
__global float *b51,
__global float *b52,
__global float *b53,
__global float *b54,
const int n5,
__global float *b60,
__global float *b61,
__global float *b62,
__global float *b63,
__global float *b64,
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
