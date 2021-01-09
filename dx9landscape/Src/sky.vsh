vs_2_x

//c0:geo transformation

def c10, 1, 4, 0, 0
def c11, 1, 1, 1, 1
def c20, 0, 0, 0, 0

dcl_position v0
dcl_texcoord v7

//-----------------
abs r0.y, v0.y
mul r0.y, r0.y, c10.y
min r0.y, r0.y, c10.x
mov r1, c11
mov r1.a, r0.y
//----------------
//mov oD0, r1.a
mov oD0, r1

m4x4 r0, v0, c0
mov oPos, r0

mov oT0, v7