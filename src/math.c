#include <math.h>

#define PI_32 3.1415926535897932384626f
#define degToRad(deg) ((deg)*PI_32 / 180.0f)

r32 clampf(r32 value, r32 min, r32 max) {
  return (value < min) ? min : ((value > max) ? max : value);
}

void identityMatrix4f(MATRIX4F *m) {
  (*m)[0] = 1;
  (*m)[1] = 0;
  (*m)[2] = 0;
  (*m)[3] = 0;
  (*m)[4] = 0;
  (*m)[5] = 1;
  (*m)[6] = 0;
  (*m)[7] = 0;
  (*m)[8] = 0;
  (*m)[9] = 0;
  (*m)[10] = 1;
  (*m)[11] = 0;
  (*m)[12] = 0;
  (*m)[13] = 0;
  (*m)[14] = 0;
  (*m)[15] = 1;
}

void perspectiveMatrix4f(MATRIX4F *m, r32 aspect, r32 fov, r32 znear, r32 zfar) {
  identityMatrix4f(m);
  (*m)[0] = 1.0f / (aspect*tanf(degToRad(fov / 2.0f)));
  (*m)[5] = 1.0f / tanf(degToRad(fov / 2.0f));
  (*m)[10] = (-znear - zfar) / (znear - zfar);
  (*m)[11] = (2.0f*zfar*znear) / (znear - zfar);
  (*m)[14] = 1;
  (*m)[15] = 0;
}

void transformMatrix4f(MATRIX4F *m, r32 tx, r32 ty, r32 tz, r32 rx, r32 ry, r32 rz) {
  identityMatrix4f(m);
  (*m)[3] = tx;
  (*m)[7] = ty;
  (*m)[11] = tz;

  r32 srx = sinf(degToRad(rx));
  r32 crx = cosf(degToRad(rx));
  r32 sry = sinf(degToRad(ry));
  r32 cry = cosf(degToRad(ry));
  r32 srz = sinf(degToRad(rz));
  r32 crz = cosf(degToRad(rz));
  
  (*m)[0] = crz*cry;
  (*m)[1] = crz*sry*srx - srz*crx;
  (*m)[2] = crz*sry*crx + srz*srx;
  (*m)[4] = srz*cry;
  (*m)[5] = srz*sry*srx + crz*crx;
  (*m)[6] = srz*sry*crx - crz*srx;
  (*m)[8] = -sry;
  (*m)[9] = cry*srx;
  (*m)[10] = cry*crx;
}

void inverseCameraMatrix4f(MATRIX4F *m, r32 tx, r32 ty, r32 tz, r32 rx, r32 ry, r32 rz) {
  r32 srx = sinf(degToRad(rx));
  r32 crx = cosf(degToRad(rx));
  r32 sry = sinf(degToRad(ry));
  r32 cry = cosf(degToRad(ry));
  r32 srz = sinf(degToRad(rz));
  r32 crz = cosf(degToRad(rz));
  
  r32 d0 = crz*cry;
  r32 d1 = crz*sry*srx - srz*crx;
  r32 d2 = crz*sry*crx + srz*srx;
  r32 d4 = srz*cry;
  r32 d5 = srz*sry*srx + crz*crx;
  r32 d6 = srz*sry*crx - crz*srx;
  r32 d8 = -sry;
  r32 d9 = cry*srx;
  r32 d10 = cry*crx;
  
  r32 A1223 = d6*tz - d10*ty;
  r32 A0223 = d2*tz - d10*tx;
  r32 A0123 = d2*ty - d6*tx;
  r32 A1213 = d5*tz - d9*ty;
  r32 A1212 = d5*d10 - d9*d6;
  r32 A0213 = d1*tz - d9*tx;
  r32 A0212 = d1*d10 - d9*d2;
  r32 A0113 = d1*ty - d5*tx;
  r32 A0112 = d1*d6 - d5*d2;

  r32 det = d0*(d5*d10 - d9*d6) - d4*(d1*d10 - d9*d2) + d8*(d1*d6 - d5*d2);
  det = 1 / det;

  (*m)[0] = det*(d5*d10 - d9*d6);
  (*m)[4] = -det*(d4*d10 - d8*d6);
  (*m)[8] = det*(d4*d9 - d8*d5);
  (*m)[12] = 0;
  (*m)[1] = -det*(d1*d10 - d9*d2);
  (*m)[5] = det*(d0*d10 - d8*d2);
  (*m)[9] = -det*(d0*d9 - d8*d1);
  (*m)[13] = 0;
  (*m)[2] = det*(d1*d6 - d5*d2);
  (*m)[6] = -det*(d0*d6 - d4*d2);
  (*m)[10] = det*(d0*d5 - d4*d1);
  (*m)[14] = 0;
  (*m)[3] = -det*(d1*A1223 - d5*A0223 + d9*A0123);
  (*m)[7] = det*(d0*A1223 - d4*A0223 + d8*A0123);
  (*m)[11] = -det*(d0*A1213 - d4*A0213 + d8*A0113);
  (*m)[15] = det*(d0*A1212 - d4*A0212 + d8*A0112);
}
