#ifndef AcumioServer_util_constants_h
#define AcumioServer_util_constants_h
//============================================================================
// Name        : util_constants.h
// Author      : Bill Province (bill@acumio.com)
// Version     :
// Copyright   : Copyright (C) 2016 Acumio
// Description : Some important constants that do not really belong in the
//               acumio::model namespace. Constants important to the
//               acumio::model namespace belong in model_constants.[h|c].
//
//               The constants provided here are important at a systems
//               level.
//============================================================================

namespace acumio {
namespace string {

// The constant char: '\0'. We have it here because sometimes we need
// to refer to it by a const reference.
extern const char NULL_TERMINATOR;

// Single-character strings of length 1 (or length 0 in the case of the
// empty string beginning with character 0).
//
// At the end, we also provide an array to look up these strings.
extern const char* const Cx00;
extern const char* const Cx01;
extern const char* const Cx02;
extern const char* const Cx03;
extern const char* const Cx04;
extern const char* const Cx05;
extern const char* const Cx06;
extern const char* const Cx07;
extern const char* const Cx08;
extern const char* const Cx09;
extern const char* const Cx0A;
extern const char* const Cx0B;
extern const char* const Cx0C;
extern const char* const Cx0D;
extern const char* const Cx0E;
extern const char* const Cx0F;
extern const char* const Cx10;
extern const char* const Cx11;
extern const char* const Cx12;
extern const char* const Cx13;
extern const char* const Cx14;
extern const char* const Cx15;
extern const char* const Cx16;
extern const char* const Cx17;
extern const char* const Cx18;
extern const char* const Cx19;
extern const char* const Cx1A;
extern const char* const Cx1B;
extern const char* const Cx1C;
extern const char* const Cx1D;
extern const char* const Cx1E;
extern const char* const Cx1F;
extern const char* const Cx20;
extern const char* const Cx21;
extern const char* const Cx22;
extern const char* const Cx23;
extern const char* const Cx24;
extern const char* const Cx25;
extern const char* const Cx26;
extern const char* const Cx27;
extern const char* const Cx28;
extern const char* const Cx29;
extern const char* const Cx2A;
extern const char* const Cx2B;
extern const char* const Cx2C;
extern const char* const Cx2D;
extern const char* const Cx2E;
extern const char* const Cx2F;
extern const char* const Cx30;
extern const char* const Cx31;
extern const char* const Cx32;
extern const char* const Cx33;
extern const char* const Cx34;
extern const char* const Cx35;
extern const char* const Cx36;
extern const char* const Cx37;
extern const char* const Cx38;
extern const char* const Cx39;
extern const char* const Cx3A;
extern const char* const Cx3B;
extern const char* const Cx3C;
extern const char* const Cx3D;
extern const char* const Cx3E;
extern const char* const Cx3F;
extern const char* const Cx40;
extern const char* const Cx41;
extern const char* const Cx42;
extern const char* const Cx43;
extern const char* const Cx44;
extern const char* const Cx45;
extern const char* const Cx46;
extern const char* const Cx47;
extern const char* const Cx48;
extern const char* const Cx49;
extern const char* const Cx4A;
extern const char* const Cx4B;
extern const char* const Cx4C;
extern const char* const Cx4D;
extern const char* const Cx4E;
extern const char* const Cx4F;
extern const char* const Cx50;
extern const char* const Cx51;
extern const char* const Cx52;
extern const char* const Cx53;
extern const char* const Cx54;
extern const char* const Cx55;
extern const char* const Cx56;
extern const char* const Cx57;
extern const char* const Cx58;
extern const char* const Cx59;
extern const char* const Cx5A;
extern const char* const Cx5B;
extern const char* const Cx5C;
extern const char* const Cx5D;
extern const char* const Cx5E;
extern const char* const Cx5F;
extern const char* const Cx60;
extern const char* const Cx61;
extern const char* const Cx62;
extern const char* const Cx63;
extern const char* const Cx64;
extern const char* const Cx65;
extern const char* const Cx66;
extern const char* const Cx67;
extern const char* const Cx68;
extern const char* const Cx69;
extern const char* const Cx6A;
extern const char* const Cx6B;
extern const char* const Cx6C;
extern const char* const Cx6D;
extern const char* const Cx6E;
extern const char* const Cx6F;
extern const char* const Cx70;
extern const char* const Cx71;
extern const char* const Cx72;
extern const char* const Cx73;
extern const char* const Cx74;
extern const char* const Cx75;
extern const char* const Cx76;
extern const char* const Cx77;
extern const char* const Cx78;
extern const char* const Cx79;
extern const char* const Cx7A;
extern const char* const Cx7B;
extern const char* const Cx7C;
extern const char* const Cx7D;
extern const char* const Cx7E;
extern const char* const Cx7F;
extern const char* const Cx80;
extern const char* const Cx81;
extern const char* const Cx82;
extern const char* const Cx83;
extern const char* const Cx84;
extern const char* const Cx85;
extern const char* const Cx86;
extern const char* const Cx87;
extern const char* const Cx88;
extern const char* const Cx89;
extern const char* const Cx8A;
extern const char* const Cx8B;
extern const char* const Cx8C;
extern const char* const Cx8D;
extern const char* const Cx8E;
extern const char* const Cx8F;
extern const char* const Cx90;
extern const char* const Cx91;
extern const char* const Cx92;
extern const char* const Cx93;
extern const char* const Cx94;
extern const char* const Cx95;
extern const char* const Cx96;
extern const char* const Cx97;
extern const char* const Cx98;
extern const char* const Cx99;
extern const char* const Cx9A;
extern const char* const Cx9B;
extern const char* const Cx9C;
extern const char* const Cx9D;
extern const char* const Cx9E;
extern const char* const Cx9F;
extern const char* const CxA0;
extern const char* const CxA1;
extern const char* const CxA2;
extern const char* const CxA3;
extern const char* const CxA4;
extern const char* const CxA5;
extern const char* const CxA6;
extern const char* const CxA7;
extern const char* const CxA8;
extern const char* const CxA9;
extern const char* const CxAA;
extern const char* const CxAB;
extern const char* const CxAC;
extern const char* const CxAD;
extern const char* const CxAE;
extern const char* const CxAF;
extern const char* const CxB0;
extern const char* const CxB1;
extern const char* const CxB2;
extern const char* const CxB3;
extern const char* const CxB4;
extern const char* const CxB5;
extern const char* const CxB6;
extern const char* const CxB7;
extern const char* const CxB8;
extern const char* const CxB9;
extern const char* const CxBA;
extern const char* const CxBB;
extern const char* const CxBC;
extern const char* const CxBD;
extern const char* const CxBE;
extern const char* const CxBF;
extern const char* const CxC0;
extern const char* const CxC1;
extern const char* const CxC2;
extern const char* const CxC3;
extern const char* const CxC4;
extern const char* const CxC5;
extern const char* const CxC6;
extern const char* const CxC7;
extern const char* const CxC8;
extern const char* const CxC9;
extern const char* const CxCA;
extern const char* const CxCB;
extern const char* const CxCC;
extern const char* const CxCD;
extern const char* const CxCE;
extern const char* const CxCF;
extern const char* const CxD0;
extern const char* const CxD1;
extern const char* const CxD2;
extern const char* const CxD3;
extern const char* const CxD4;
extern const char* const CxD5;
extern const char* const CxD6;
extern const char* const CxD7;
extern const char* const CxD8;
extern const char* const CxD9;
extern const char* const CxDA;
extern const char* const CxDB;
extern const char* const CxDC;
extern const char* const CxDD;
extern const char* const CxDE;
extern const char* const CxDF;
extern const char* const CxE0;
extern const char* const CxE1;
extern const char* const CxE2;
extern const char* const CxE3;
extern const char* const CxE4;
extern const char* const CxE5;
extern const char* const CxE6;
extern const char* const CxE7;
extern const char* const CxE8;
extern const char* const CxE9;
extern const char* const CxEA;
extern const char* const CxEB;
extern const char* const CxEC;
extern const char* const CxED;
extern const char* const CxEE;
extern const char* const CxEF;
extern const char* const CxF0;
extern const char* const CxF1;
extern const char* const CxF2;
extern const char* const CxF3;
extern const char* const CxF4;
extern const char* const CxF5;
extern const char* const CxF6;
extern const char* const CxF7;
extern const char* const CxF8;
extern const char* const CxF9;
extern const char* const CxFA;
extern const char* const CxFB;
extern const char* const CxFC;
extern const char* const CxFD;
extern const char* const CxFE;
extern const char* const CxFF;

extern const char* const LETTER_STRINGS[256];
} // namespace model
} // namespace acumio

#endif //AcumioServer_util_constants_h
