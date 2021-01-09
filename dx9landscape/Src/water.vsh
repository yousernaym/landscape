vs_2_x
//c4, c7: tcoord scaling
//c5.xyz: light direction
//c6.xyz: camera position
//c8-c11: reflection matrix

def c20, 0, 0, 0, 0
def c21, 1, 1, 1, 1

def c29, 0, 1, -1, -1
def c30, 0.5f, 0.02f, 0, 0
def c31, 2, 0, 0, 0

dcl_position v0
dcl_normal v1
dcl_tangent v2
dcl_binormal v3

//Fog---------------------------------
mov r0.y, c6.y
if_lt r0.y, c20.y   //camera under water
	sub r0, v0, c6
	dp3 r1.x, r0, r0
	pow r1.x, r1.x, c30.x  //distance to vertex
	
	mul r1.x, r1.x, c30.y
	sub r1.x, c21.x, r1.x
	min r1.x, r1.x, c21.x
	max r1.x, r1.x, c20.x
else
	mov r1.x, c21.x
endif
mov oFog, r1.x
//------------------------------

//lighting---------------------------
  //tangent transformations
	//light
	/*dp3 r0.x, v2, c5
	dp3 r0.y, v1, c5
	dp3 r0.z, v3, c5
	*/
	//cam pos
	sub r1, c6, v0
	dp3 r2.x, v2, r1
	dp3 r2.y, v1, r1
	dp3 r2.z, v3, r1
	//nrm r1.xyz, r2.xyz
	//mov r1.w, c20.x

//mov oT3, r0.xyz
//mov oT4, r1.xyz
mov oT4, r2.xyz
//------------------------------

//geo transformations---
m4x4 r0, v0, c0
mov oPos, r0
//------------------------

//local reflection coords-------
/*mul r1.xz, v1.xz, c31.xx    //amount of distortion
mov r2, v0
add r2.xz, v0.xz, r1.xz
m4x4 r0, r2, c8

mov oT6, r0*/
mov oT6, v0
//-----------------------------------

//tex transformations----------------
mov r0.xyzw, v0.xzxz
mul r1, r0, c4
mul r2, r0, c7

mov oT0, r1.zw  //identity
mov oT1, r1
mov oT2, r2
//t3, t4 reserved for tangent space stuff
mov oT5, r2.zw
//--------------------------------------

/*mov r1.xy, r1.zw
mov r1.zw, c29.xy
add r1.xy, r1.xy, c29.zw

mov oPos, r1*/
