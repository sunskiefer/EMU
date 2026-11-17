// Generated from vendor/lemus/qvgt/core/netlists/*.cir by tools/gen_netlists.py.
#pragma once

#include <cstring>

namespace qvgt::netlists {

inline constexpr const char *gateout = R"cir(* QuadraVerb GT preamp, after the gate (sheet 1): U4 VCA 2's output current (an input here) into
* U13D (12 +, 13 -, 14 out; R101 20K, C38 20p), the effects loop (send: R96 910, R119 100 to J7;
* return: J8 into U13B, a follower, then R120, R121), U3 picking the return (Y) or the send's signal
* (X: R99 10K, R95 6.8K), and U13C (10 +, 9 -, 8 out; R102 10K): to the A/D (sheet 2, U16 pin 3).
* With nothing in J8 the tip is R118's 0 V, and J8's switch grounds U3's select (the loop out
* whatever U11 says: core/preamp.cpp).
* The TL084s without their slew limit (13 V/us; audio needs 1.3 at most): with it the solver fails
* where a stage clips, and leaving it out moves a clipping stage's output 80-94 dB under the signal
* against ngspice with it (docs/circuit.md).
.param gbw=3meg gain=2e5 sat=10.5 sr=0 ron=125
* Each op-amp's input noise as a source in series with its + input (input noise_<part>, 0 V
* unless HISS is on: core/preamp.cpp).
Ivca 0 g13 input vca
X13D nz_13d g13 u13d opamp {gbw} {gain} {sat} {sr}
VNZ_13D nz_13d 0 input noise_13d
R101 g13 u13d 20k
C38 g13 u13d 20p
R96 u13d snd 910
R119 snd 0 100
Vret ret 0 input ret
R118 ret 0 47k
R123 ret r5 1k
X13B nz_13b r7 r7 opamp {gbw} {gain} {sat} {sr}
VNZ_13B nz_13b r5 input noise_13b
R120 r7 yy 1k
R121 yy 0 680
R99B u13d xx 10k
R95 xx 0 6.8k
S3Y yy n9c loop {ron} 1e9
S3X xx n9c !loop {ron} 1e9
X13C nz_13c n9c out opamp {gbw} {gain} {sat} {sr}
VNZ_13C nz_13c 0 input noise_13c
R102 n9c out 10k
.output out out
.output send snd
.control loop 0
)cir";

inline constexpr const char *input = R"cir(* QuadraVerb GT preamp, the input (sheet 1, upper left): LEFT and RIGHT INPUT (J10, J11), U1's
* followers, the INPUT pot (dual 10KB) setting the gain of U2's inverting stages (0 at the left end,
* -10 at the right: 10K against 1K + the pot's track), and U2C's mono sum (R7, R10 470) with a gain
* of 6 (R9 10K / R8 2K). Outputs: p8 (U2 pin 8: the compressor, the gate's detector, the headroom
* LEDs) and the two stages' outputs for the dry path (to U12 on sheet 2).
* The TL084s without their slew limit (13 V/us; audio needs 1.3 at most): with it the solver fails
* where a stage clips, and leaving it out moves a clipping stage's output 80-94 dB under the signal
* against ngspice with it (docs/circuit.md).
.param gbw=3meg gain=2e5 sat=10.5 sr=0
* Each op-amp's input noise as a source in series with its + input (input noise_<part>, 0 V
* unless HISS is on: core/preamp.cpp).
Vinl inl 0 input left
Vinr inr 0 input right
* left: J10 tip, R3 470K to ground, R2 1K into U1D (12 +, 13 -, 14 out) as a follower
R3 inl 0 470k
R2 inl u1dp 1k
X1D nz_1d u1do u1do opamp {gbw} {gain} {sat} {sr}
VNZ_1D nz_1d u1dp input noise_1d
R4 u1do n4 1k
C2 n4 n5 10u
* the INPUT pot's left section: pin 1 (n5) - wiper 5 - pin 3 (U2D's output); clockwise moves the
* wiper to pin 1, the gain's top
P1A n5 w1 u2do 10k input
X2D nz_2d w1 u2do opamp {gbw} {gain} {sat} {sr}
VNZ_2D nz_2d 0 input noise_2d
* right: J11, R1 470K, R5 1K, U1A (3 +, 2 -, 1 out)
R1 inr 0 470k
R5 inr u1ap 1k
X1A nz_1a u1ao u1ao opamp {gbw} {gain} {sat} {sr}
VNZ_1A nz_1a u1ap input noise_1a
R6 u1ao n6 1k
C1 n6 n7 10u
P1B n7 w2 u2ao 10k input
X2A nz_2a w2 u2ao opamp {gbw} {gain} {sat} {sr}
VNZ_2A nz_2a 0 input noise_2a
* the mono sum into U2C (10 +, 9 -, 8 out)
R7 u2do m 470
R10 u2ao m 470
X2C nz_2c n9 p8 opamp {gbw} {gain} {sat} {sr}
VNZ_2C nz_2c m input noise_2c
R8 n9 0 2k
R9 n9 p8 10k
.output p8 p8
.output dryl u2do
.output dryr u2ao
.control input 1
)cir";

inline constexpr const char *models = R"cir(* Semiconductor models for the QuadraVerb GT's netlists (DC equations: the solver has no charge
* storage). Parts from the GT service manual's parts list (pp. 26-28) and their data sheets
* (private/docs/datasheets/); docs/circuit.md says where each figure comes from.
* 1N4148 (D1-D19, D25-D27): the common SPICE fit of the Philips/onsemi curve (0.62 V at 1 mA, 0.72 V
* at 10 mA).
.model 1N4148 D(IS=2.52n N=1.752 RS=0.568)
* Strings of them in series, as one diode each: the same current through identical junctions, so
* N and RS add up (the solver's diodes store no charge, so this is exact).
.model 1N4148X2 D(IS=2.52n N=3.504 RS=1.136)
.model 1N4148X3 D(IS=2.52n N=5.256 RS=1.704)
* 2N5460 (Q1), P-channel JFET: V_GS(off) 0.75-6 V, I_DSS 1-5 mA (onsemi 2N5460/D); the sheet's
* typical curves are drawn for V_GS(off) 2 V with I_DSS 3 mA: VTO -2, BETA = I_DSS / VTO^2.
.model 2N5460 PJF(VTO=-2 BETA=0.75m LAMBDA=0.01 IS=1e-14 N=1)
)cir";

inline constexpr const char *overdrive = R"cir(* QuadraVerb GT preamp, the overdrive (sheet 1): U4 VCA 1's output current (an input here, the
* VCA itself is core/ssm2120.h) into U1C's virtual ground (10 +, 9 -, 8 out). U3B (CD4053) picks
* the feedback: off (od < 0.5), Bx: R39 36K from the output (R40 10K from that node to ground);
* on, By: R38 4.7K into the T-node (R37 1K and C11 4.7u to ground) and R36 47K across two diode
* strings, D6-D8 conducting toward U1C's output side, D9-D10 back (sheet 1 draws them so). C12 20p
* across. The switches' on resistance is the CD4053B's at 17 V (125 ohms typical).
* The TL084s without their slew limit (13 V/us; audio needs 1.3 at most): with it the solver fails
* where a stage clips, and leaving it out moves a clipping stage's output 80-94 dB under the signal
* against ngspice with it (docs/circuit.md).
.param gbw=3meg gain=2e5 sat=10.5 sr=0 ron=125
* Each op-amp's input noise as a source in series with its + input (input noise_<part>, 0 V
* unless HISS is on: core/preamp.cpp).
Ivca 0 n9 input vca
X1C nz_1c n9 od8 opamp {gbw} {gain} {sat} {sr}
VNZ_1C nz_1c 0 input noise_1c
C12 n9 od8 20p
R39 od8 nbx 36k
R40 nbx 0 10k
S3BX nbx n9 !od {ron} 1e9
R38 od8 nb 4.7k
R37 nb n37 1k
C11 n37 0 4.7u
R36 nb nby 47k
* D6-D8 and D9-D10, each string as one diode (models.cir)
D678 nby nb 1N4148X3
D910 nb nby 1N4148X2
S3BY nby n9 od {ron} 1e9
.output od8 od8
.control od 0
)cir";

inline constexpr const char *shaper = R"cir(* QuadraVerb GT preamp, from the overdrive's output to the gate (sheet 1): the distortion, the preamp
* tone, the cabinet simulator and the bass boost, and U13A, which drives the gate's VCA. The CD4053
* and CD4051 switches as on resistances (125 ohms, the CD4053B's at 17 V), each closed by its
* control: a 4053 section's Y input while its select is high. U4's control output (the compressor's)
* moves Q1's gate through R75.
* The TL084s without their slew limit (13 V/us; audio needs 1.3 at most): with it the solver fails
* where a stage clips, and leaving it out moves a clipping stage's output 80-94 dB under the signal
* against ngspice with it (docs/circuit.md).
.param gbw=3meg gain=2e5 sat=10.5 sr=0 ron=125
* Each op-amp's input noise as a source in series with its + input (input noise_<part>, 0 V
* unless HISS is on: core/preamp.cpp).
Vod in 0 input od
Vcon con 0 input conout
Vp12 p12 0 12
*
* Distortion. U1B (5 +, 6 -, 7 out): a multiple-feedback band-pass ahead of the clipper; its output
* (R14 1.5K) and the stage's input (R32 3.3K) into C8, U6A's input (3 +, 2 -, 1 out).
R31 in a1 15k
C6 a1 n6 .01u
C5 a1 u1b .01u
R13 n6 u1b 36k
X1B nz_1b n6 u1b opamp {gbw} {gain} {sat} {sr}
VNZ_1B nz_1b 0 input noise_1b
R14 u1b s1 1.5k
R32 in s1 3.3k
C8 s1 n2 4.7u
X6A nz_6a n2 d1 opamp {gbw} {gain} {sat} {sr}
VNZ_6A nz_6a 0 input noise_6a
* The type: eight rows, a feedback resistor (left) and an output attenuator (right); U8 (CD4051)
* joins the selected row's middle to U6A's output. DIST 1-3 pick channel ch0-ch7 (docs/firmware.md).
RL0 n2 j0 220k
RR0 j0 tap 9.1k
RL1 n2 j1 75k
RR1 j1 tap 11k
RL2 n2 j2 36k
RR2 j2 tap 12k
RL3 n2 j3 15k
RR3 j3 tap 12k
RL4 n2 j4 7.5k
RR4 j4 tap 11k
RL5 n2 j5 3.9k
RR5 j5 tap 9.1k
RL6 n2 j6 2.2k
RR6 j6 tap 7.5k
RL7 n2 j7 1k
RR7 j7 tap 3.9k
S8C0 j0 d1 ch0 {ron} 1e9
S8C1 j1 d1 ch1 {ron} 1e9
S8C2 j2 d1 ch2 {ron} 1e9
S8C3 j3 d1 ch3 {ron} 1e9
S8C4 j4 d1 ch4 {ron} 1e9
S8C5 j5 d1 ch5 {ron} 1e9
S8C6 j6 d1 ch6 {ron} 1e9
S8C7 j7 d1 ch7 {ron} 1e9
* the clipping diodes across U6A: D5, D4 from its input to its output; D3, D2, D1 back
* (each string as one diode, models.cir)
D54 n2 d1 1N4148X2
D321 d1 n2 1N4148X3
* Q1 with C7 2200p across U6A, its gate from +12 (R41 390K), ground (R11 220K) and U4's control
* output (R75 220K): it conducts, and takes the highs off, as the control output falls
J1 jd g n2 2N5460
C7 jd d1 2200p
R41 p12 g 390k
R11 g 0 220k
R75 con g 220k
* U9A (DIST ON): Ay the distortion (the rows' common), Ax the clean signal (R33 10K, R12 6.8K)
R33 in ax 10k
R12 ax 0 6.8k
S9AY tap v2 dist {ron} 1e9
S9AX ax v2 !dist {ron} 1e9
X7A nz_7a v2 t1 opamp {gbw} {gain} {sat} {sr}
VNZ_7A nz_7a 0 input noise_7a
R57 v2 t1 10k
*
* Preamp tone. Presence, U7D (12 +, 13 -, 14 out), then bright, U7C (10 +, 9 -, 8 out), from it;
* U9C picks presence (Y) or bright, U9A flat (Y: R58, R60 from U7A) or the tone; U7B (5 +, 6 -,
* 7 out) the sum's inverter.
C24 t1 a1t .033u
R79 a1t a2t 56k
C23 a2t n13 .01u
C22 a2t a4t .01u
R77 a4t n13 9.1k
R78 a4t t2 1k
R76 a4t 0 47
R80 a1t a3t 56k
R87 a3t 0 5.6k
X7D nz_7d n13 t2 opamp {gbw} {gain} {sat} {sr}
VNZ_7D nz_7d a3t input noise_7d
R91 t2 b2t 56k
C34 b2t n9b .01u
C35 b2t b4t .01u
R89 b4t n9b 9.1k
R99A b4t t3 1k
R84 b4t 0 100
R88 t2 b3t 7.5k
R85 b3t 0 1k
X7C nz_7c n9b t3 opamp {gbw} {gain} {sat} {sr}
VNZ_7C nz_7c b3t input noise_7c
R58 t1 f1 10k
R60 f1 0 6.8k
R92 t2 p1 4.7k
R94 p1 0 3.3k
R93 t3 q1 3.9k
R113 q1 0 1.6k
S9CY p1 cc presence {ron} 1e9
S9CX q1 cc !presence {ron} 1e9
S9AY2 f1 v6 flat {ron} 1e9
S9AX2 cc v6 !flat {ron} 1e9
X7B nz_7b v6 s opamp {gbw} {gain} {sat} {sr}
VNZ_7B nz_7b 0 input noise_7b
R59 v6 s 10k
*
* Cabinet simulator: two unity-gain Sallen-Key low-passes (U6C 10 +, 9 -, 8 out; U6D 12 +, 13 -,
* 14 out), each tapped through 30K/20K; U5B picks cabinet 1 (one section) or 2 (both), U5C the
* cabinet or the plain signal (R53, R50), into U13A's virtual ground.
R51 s c1 10k
R48 c1 c2 10k
C16 c2 0 2700p
C17 c1 k8 5600p
X6C nz_6c k8 k8 opamp {gbw} {gain} {sat} {sr}
VNZ_6C nz_6c c2 input noise_6c
R44 k8 c3 10k
R47 c3 c4 10k
C15 c4 0 2700p
C14 c3 k14 5600p
X6D nz_6d k14 k14 opamp {gbw} {gain} {sat} {sr}
VNZ_6D nz_6d c4 input noise_6d
R46 k14 by 30k
R45 by 0 20k
R43 k8 bx 30k
R42 bx 0 20k
R53 s cx 30k
R50 cx 0 20k
S5BY by bo cab2 {ron} 1e9
S5BX bx bo !cab2 {ron} 1e9
S5CY bo m cab {ron} 1e9
S5CX cx m !cab {ron} 1e9
* Bass boost: U6B (5 +, 6 -, 7 out) a multiple-feedback band-pass from the cabinet's input (R56,
* R61, R55), into U13A's virtual ground through U5A (R52; Ax not connected)
R56 s qb 2.2k
R61 qb 0 750
R55 qb pb 39k
C9 pb n6b .01u
C10 pb o7 .01u
R54 o7 n6b 300k
X6B nz_6b n6b o7 opamp {gbw} {gain} {sat} {sr}
VNZ_6B nz_6b 0 input noise_6b
R52 o7 ay 10k
R49 ay 0 6.8k
S5AY ay m bass {ron} 1e9
* U13A (3 +, 2 -, 1 out): to the gate's VCA (R103, C27: core/ssm2120.h)
X13A nz_13a m pre opamp {gbw} {gain} {sat} {sr}
VNZ_13A nz_13a 0 input noise_13a
R105 m pre 15k
*
* Partitions (core/preamp.cpp runs each as its own circuit, in this order): joined only at op-amp
* outputs, which the model drives as ideal sources, so the split solves the same equations; it
* keeps the Newton solves to the clipper's few nodes. The cabinet, bass and sum stay in linear mode
* throughout, so they are one; presence and bright leave it now and then, and each relinearises on
* its own (joined to the tone stack, their error in linear mode on program 5 rose 14 dB).
.part bandpass R31 C6 C5 R13 X1B
.part clipper in=u1b R14 R32 C8 X6A RL0 RR0 RL1 RR1 RL2 RR2 RL3 RR3 RL4 RR4 RL5 RR5 RL6 RR6 RL7 RR7
+ S8C0 S8C1 S8C2 S8C3 S8C4 S8C5 S8C6 S8C7 D54 D321 J1 C7 R41 R11 R75 R33 R12 S9AY S9AX X7A R57
.part presence in=t1 C24 R79 C23 C22 R77 R78 R76 R80 R87 X7D
.part bright in=t2 R91 C34 C35 R89 R99A R84 R88 R85 X7C
.part tone in=t1,t2,t3 R58 R60 R92 R94 R93 R113 S9CY S9CX S9AY2 S9AX2 X7B R59
.part cabinet in=s R51 R48 C16 C17 X6C R44 R47 C15 C14 X6D R56 R61 R55 C9 C10 R54 X6B
+ R46 R45 R43 R42 R53 R50 S5BY S5BX S5CY S5CX R52 R49 S5AY X13A R105
.output pre pre
.output dist d1
.control dist 0
.control ch0 0
.control ch1 0
.control ch2 0
.control ch3 0
.control ch4 0
.control ch5 0
.control ch6 0
.control ch7 1
.control presence 1
.control flat 1
.control cab 0
.control cab2 0
.control bass 0
)cir";

inline const char *find(const char *name)
{
	struct Entry {
		const char *name, *text;
	};
	static const Entry all[] = {
		{"gateout", gateout},
		{"input", input},
		{"models", models},
		{"overdrive", overdrive},
		{"shaper", shaper},
	};
	for (const auto &e : all)
		if (std::strcmp(e.name, name) == 0)
			return e.text;
	return nullptr;
}

} // namespace qvgt::netlists
