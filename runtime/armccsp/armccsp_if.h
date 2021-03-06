/*
 *	armccsp_if.h -- ARM/CCSP public interface for CIF style code
 *	Copyright (C) 2013-2015 Fred Barnes, University of Kent <frmb@kent.ac.uk>
 *
 *	This program is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	This program is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program; if not, write to the Free Software
 *	Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __ARMCCSP_IF_H
#define __ARMCCSP_IF_H

/*{{{  types (that need to be visible to applications using CIF/CCSP only)*/
typedef uint32_t *Workspace;
typedef void *Channel;
typedef uint32_t word;

typedef struct TAG_light_proc_barrier {
	Workspace succ;
	int count;
} LightProcBarrier;

/*}}}*/
/*{{{  kernel call constants, must stay consistent with table in kernel.c*/
#define CALL_CHANOUT 0
#define CALL_CHANIN 1
#define CALL_SHUTDOWN 2
#define CALL_PAUSE 3
#define CALL_SETERR 4
#define CALL_SETERRM 5
#define CALL_MALLOC 6
#define CALL_MRELEASE 7
#define CALL_RUNP 8
#define CALL_STOPP 9
#define CALL_LDTIMER 10
#define CALL_TIN 11
#define CALL_ALT 12
#define CALL_ALTEND 13
#define CALL_ENBC 14
#define CALL_DISC 15
#define CALL_ALTWT 16
#define CALL_TALT 17
#define CALL_ENBT 18
#define CALL_DIST 19
#define CALL_TALTWT 20
#define CALL_TESTCHAN 21

/*}}}*/
/*{{{  functions that should only ever be called in the context of the main program, i.e. from main()*/
extern int ccsp_init (void);
extern Workspace ProcAllocInitial (const int paramwords, const int stackwords);

extern void ProcStartInitial_blind (Workspace p, void (*fcn)(Workspace));
#define ProcStartInitial(P,FCN) ProcStartInitial_blind(P,(void (*)(Workspace))FCN)

#define WORKSPACE_SIZE(args,stack) \
	((args) + (stack) + 16)

/*}}}*/

/*{{{  ENTER_KERNEL(proc,call,args): abstraction for kernel entry proper*/

/* @APICALLCHAIN: ENTER_KERNEL: =8 */

#define ENTER_KERNEL(p,c,a) do { \
	__asm__ __volatile__ ("							\n" \
		"	mov	r0, %0						\n" \
		"	add	r3, r0, #4		@ r3 = &(p->stack)	\n" \
		"	push	{lr}			@ save link-register	\n" \
		"	str	sp, [r3, #0]		@ p->stack = sp		\n" \
		"	add	r3, r0, #8		@ r3 = &(p->raddr)	\n" \
		"	adr	r2, 0f						\n" \
		"	str	r2, [r3, #0]		@ p->raddr = &.Lcont	\n" \
		"								\n" \
		"	ldr	r2, [r0, #0]		@ r2 = (p->sched)	\n" \
		"	ldr	r3, [r2, #0]		@ r3 = p->sched->stack	\n" \
		"	mov	sp, r3			@ switch stacks		\n" \
		"	mov	r1, %1						\n" \
		"	mov	r2, %2						\n" \
		"	bl	ccsp_entry(PLT)					\n" \
		"0:								\n" \
		"					@ note: when we get back\n" \
		"					@ r0 = process-desc	\n" \
		"	add	r3, r0, #4		@ r3 = &(p->stack)	\n" \
		"	ldr	r1, [r3, #0]		@ r1 = p->stack		\n" \
		"	mov	sp, r1			@ switch back		\n" \
		"	pop	{lr}			@ restore link-register	\n" \
		"								\n" \
		: : "r" (p), "r" (c), "r" (a) \
		: "memory", "cc", "r0", "r1", "r2", "r3"); \
	} while (0)

/*}}}*/
/*{{{  EXTERNAL_CALL(func,stack,result): abstraction for external function call*/

/* @APICALLCHAIN: EXTERNAL_CALL: =20 */

#define EXTERNAL_CALL(f,s,r) do { \
	__asm__ __volatile__ ("							\n" \
		"	mov	r4, sp			@ save SP here		\n" \
		"	mov	sp, %2			@ new SP		\n" \
		"	ldr	r0, [sp, #0]		@ parameters		\n" \
		"	ldr	r1, [sp, #4]					\n" \
		"	ldr	r2, [sp, #8]					\n" \
		"	ldr	r3, [sp, #12]					\n" \
		"	add	sp, sp, #16		@ adjust for reg-params	\n" \
		"	mov	lr, pc						\n" \
		"	bx	%1						\n" \
		"	mov	sp, r4			@ restore previous SP	\n" \
		"	mov	%0, r0			@ save result		\n" \
		: "=r" (r) \
		: "r" (f), "r" (s) \
		: "memory", "cc", "r0", "r1", "r2", "r3", "r4", "lr"); \
	} while (0)
/*}}}*/

/*
 * BELOW: the CIF API for ARM/CCSP.
 */

/*{{{  static inline void ChanInit (Workspace p, Channel *c)*/
/* @APICALLCHAIN: ChanInit: =? */
/*
 *	initialises a channel (blanks it).
 */
static inline void ChanInit (Workspace p, Channel *c)
{
	*c = NULL;
}
/*}}}*/
/*{{{  static inline void ChanOut (Workspace p, Channel *c, const void *ptr, const int bytes)*/
/* @APICALLCHAIN: ChanOut: =?, ENTER_KERNEL */
/*
 *	channel output.
 */
static inline void ChanOut (Workspace p, Channel *c, const void *ptr, const int bytes)
{
	void *dargs[4] = {(void *)p, (void *)c, (void *)ptr, (void *)bytes};
	int call = CALL_CHANOUT;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void ChanIn (Workspace p, Channel *c, void *ptr, const int bytes)*/
/* @APICALLCHAIN: ChanIn: =?, ENTER_KERNEL */
/*
 *	channel input.
 */
static inline void ChanIn (Workspace p, Channel *c, void *ptr, const int bytes)
{
	void *dargs[4] = {(void *)p, (void *)c, ptr, (void *)bytes};
	int call = CALL_CHANIN;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void Reschedule (Workspace p)*/
/* @APICALLCHAIN: Reschedule: =?, ENTER_KERNEL */
/*
 *	reschedules -- scheduler yield.
 */
static inline void Reschedule (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_PAUSE;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void SetErr (Workspace p)*/
/* @APICALLCHAIN: SetErr: =?, ENTER_KERNEL */
/*
 *	hard run-time error.
 */
static inline void SetErr (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_SETERR;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void SetErrM (Workspace p, const char *msg)*/
/* @APICALLCHAIN: SetErrM: =?, ENTER_KERNEL */
/*
 *	hard run-time error (with message).
 */
static inline void SetErrM (Workspace p, const char *msg)
{
	void *dargs[2] = {(void *)p, (void *)msg};
	int call = CALL_SETERRM;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void Shutdown (Workspace p)*/
/* @APICALLCHAIN: Shutdown: =?, ENTER_KERNEL */
/*
 *	shuts down the run-time system; last thing an application does.
 */
static inline void Shutdown (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_SHUTDOWN;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void *MAlloc (Workspace p, const int bytes)*/
/* @APICALLCHAIN: MAlloc: =?, ENTER_KERNEL */
/*
 *	dynamic memory allocation.
 */
static inline void *MAlloc (Workspace p, const int bytes)
{
	void *ptr;
	void *dargs[3] = {(void *)p, (void *)bytes, (void **)&ptr};
	int call = CALL_MALLOC;

	ENTER_KERNEL (p, call, dargs);

	return ptr;
}
/*}}}*/
/*{{{  static inline void MRelease (Workspace p, void *ptr)*/
/* @APICALLCHAIN: MRelease: =?, ENTER_KERNEL */
/*
 *	dynamic memory release.
 */
static inline void MRelease (Workspace p, void *ptr)
{
	void *dargs[2] = {(void *)p, ptr};
	int call = CALL_MRELEASE;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void LightProcBarrierInit (Workspace p, LightProcBarrier *b, const int numprocs)*/
/* @APICALLCHAIN: LightProcBarrierInit: =? */
/*
 *	initialises a process barrier.
 */
static inline void LightProcBarrierInit (Workspace p, LightProcBarrier *b, const int numprocs)
{
	b->succ = p;
	b->count = numprocs;
}
/*}}}*/
/*{{{  static inline void RunP (Workspace p, Workspace other)*/
/* @APICALLCHAIN: RunP: =?, ENTER_KERNEL */
/*
 *	runs a process.
 */
static inline void RunP (Workspace p, Workspace other)
{
	void *dargs[2] = {(void *)p, (void *)other};
	int call = CALL_RUNP;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void StopP (Workspace p)*/
/* @APICALLCHAIN: StopP: =?, ENTER_KERNEL */
/*
 *	stops a process.
 */
static inline void StopP (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_STOPP;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void LightProcBarrierWait (Workspace p, LightProcBarrier *b)*/
/* @APICALLCHAIN: LightProcBarrierWait: =?, StopP */
/*
 *	waits for completion of a lightweight process barrier.
 *	most of the operational guts for this are in ProcStartupCode (kfunc.c)
 */
static inline void LightProcBarrierWait (Workspace p, LightProcBarrier *b)
{
	if (b->count) {
		b->succ = p;
		StopP (p);
	}
}
/*}}}*/
/*{{{  static inline int TimerRead (Workspace p)*/
/* @APICALLCHAIN: TimerRead: =?, ENTER_KERNEL */
/*
 *	reads the integer microsecond clock
 */
static inline int TimerRead (Workspace p)
{
	int tval;
	void *dargs[2] = {(void *)p, (void *)&tval};
	int call = CALL_LDTIMER;

	ENTER_KERNEL (p, call, dargs);

	return tval;
}
/*}}}*/
/*{{{  static inline int TimerWait (Workspace p, int timeout)*/
/* @APICALLCHAIN: TimerWait: =?, ENTER_KERNEL */
/*
 *	waits until the specified time has passed.
 */
static inline int TimerWait (Workspace p, int timeout)
{
	int tval = timeout;
	void *dargs[2] = {(void *)p, (void *)&tval};
	int call = CALL_TIN;

	ENTER_KERNEL (p, call, dargs);

	return tval;
}
/*}}}*/
/*{{{  static inline int Time_AFTER (int t1, int t2)*/
/* @APICALLCHAIN: Time_AFTER: =? */
/*
 *	determines whether one time (t2) is after another (t1).
 */
static inline int Time_AFTER (int t1, int t2)
{
	int temp = t2 - t1;

	if (temp & (1 << ((sizeof (int) * 8) - 1))) {
		/* high-bit set, so yes */
		return 1;
	}
	return 0;
}
/*}}}*/
/*{{{  static inline void Alt (Workspace p)*/
/* @APICALLCHAIN: Alt: =?, ENTER_KERNEL */
/*
 *	start alternative.
 */
static inline void Alt (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_ALT;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void AltEnd (Workspace p)*/
/* @APICALLCHAIN: AltEnd: =?, ENTER_KERNEL */
/*
 *	end alternative.
 */
static inline void AltEnd (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_ALTEND;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline int AltEnableChannel (Workspace p, const int guard, Channel *c)*/
/* @APICALLCHAIN: AltEnableChannel: =?, ENTER_KERNEL */
/*
 *	enable channel.
 */
static inline int AltEnableChannel (Workspace p, const int guard, Channel *c)
{
	int enabled = 0;
	void *dargs[4] = {(void *)p, (void *)guard, (void *)c, (void *)&enabled};
	int call = CALL_ENBC;

	ENTER_KERNEL (p, call, dargs);

	return enabled;
}
/*}}}*/
/*{{{  static inline int AltDisableChannel (Workspace p, const int guard, Channel *c)*/
/* @APICALLCHAIN: AltDisableChannel: =?, ENTER_KERNEL */
/*
 *	disable channel.
 */
static inline int AltDisableChannel (Workspace p, const int guard, Channel *c)
{
	int fired = 0;
	void *dargs[4] = {(void *)p, (void *)guard, (void *)c, (void *)&fired};
	int call = CALL_DISC;

	ENTER_KERNEL (p, call, dargs);

	return fired;
}
/*}}}*/
/*{{{  static inline void AltWait (Workspace p)*/
/* @APICALLCHAIN: AltWait: =?, ENTER_KERNEL */
/*
 *	waits for one of the alternatives to become ready.
 */
static inline void AltWait (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_ALTWT;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline void TAlt (Workspace p)*/
/* @APICALLCHAIN: TAlt: =?, ENTER_KERNEL */
/*
 *	start timed alternative.
 */
static inline void TAlt (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_ALT;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline int AltEnableTimeout (Workspace p, const int guard, const int timeout)*/
/* @APICALLCHAIN: AltEnableTimeout: =?, ENTER_KERNEL */
/*
 *	enable timeout.
 */
static inline int AltEnableTimeout (Workspace p, const int guard, const int timeout)
{
	int enabled = 0;
	void *dargs[4] = {(void *)p, (void *)guard, (void *)timeout, (void *)&enabled};
	int call = CALL_ENBT;

	ENTER_KERNEL (p, call, dargs);

	return enabled;
}
/*}}}*/
/*{{{  static inline int AltDisableTimeout (Workspace p, const int guard, const int timeout)*/
/* @APICALLCHAIN: AltDisableTimeout: =?, ENTER_KERNEL */
/*
 *	disable timeout.
 */
static inline int AltDisableTimeout (Workspace p, const int guard, const int timeout)
{
	int fired = 0;
	void *dargs[4] = {(void *)p, (void *)guard, (void *)timeout, (void *)&fired};
	int call = CALL_DIST;

	ENTER_KERNEL (p, call, dargs);

	return fired;
}
/*}}}*/
/*{{{  static inline void TAltWait (Workspace p)*/
/* @APICALLCHAIN: TAltWait: =?, ENTER_KERNEL */
/*
 *	waits for one of the alternatives to become ready (with timeout).
 */
static inline void TAltWait (Workspace p)
{
	void *dargs[1] = {(void *)p};
	int call = CALL_TALTWT;

	ENTER_KERNEL (p, call, dargs);
}
/*}}}*/
/*{{{  static inline int TestChan (Workspace p, Channel *c)*/
/* @APICALLCHAIN: TestChan: =?, ENTER_KERNEL */
/*
 *	tests to see if a channel is ready
 */
static inline int TestChan (Workspace p, Channel *c)
{
	int ready = 0;
	void *dargs[3] = {(void *)p, (void *)c, (void *)&ready};
	int call = CALL_TESTCHAN;

	ENTER_KERNEL (p, call, dargs);

	return ready;
}
/*}}}*/

/*{{{  other things which are not macros, but defined in kfunc.c*/
extern Workspace LightProcInit (Workspace p, word *stack, const int nparams, const int stkwords);
extern void LightProcFree (Workspace p, Workspace ws);
extern void ProcParamAny (Workspace p, Workspace other, int paramno, void *arg);
extern void *ProcGetParamAny (Workspace p, int paramno);

/* @APICALLCHAIN: ProcParam: ProcParamAny */
#define ProcParam(p,o,n,a) do { ProcParamAny (p, o, n, (void *)a); } while (0)
/* @APICALLCHAIN: ProcGetParam: ProcGetParamAny */
#define ProcGetParam(p,n,T) ((T)ProcGetParamAny (p, n))

extern void ProcPar (Workspace p, int nprocs, ...);
extern void LightProcStart (Workspace p, LightProcBarrier *bar, Workspace ws, void *fcn);

extern int ProcAlt (Workspace p, ...);
extern int ProcPriAlt (Workspace p, ...);
extern int ProcPriAltSkip (Workspace p, ...);

extern word ExternalCall0 (void *func);
extern word ExternalCall1 (void *func, word arg);
extern word ExternalCallN (void *func, word argc, ...);


/*}}}*/


/*
 * BELOW: assorted macros/etc. for use by the run-time system (keeps assembler chunks all in one place).
 */

/*{{{  static inline void ProcessResume (Workspace p)*/
/*
 *	called from the run-time to resume a process, ensures r0 = process-descriptor.
 */
static inline void ProcessResume (Workspace p)
{
	__asm__ __volatile__ ( \
		"	mov	r0, %0						\n" \
		"	add	r1, r0, #8		@ r1 = &(p->raddr)	\n" \
		"	ldr	r2, [r1, #0]		@ r2 = p->raddr		\n" \
		"	bx	r2						\n" \
		: : "r" (p) \
		: "memory", "cc", "r0", "r1", "r2", "r3");
}
/*}}}*/
/*{{{  static inline void RuntimeSaveStack (Workspace p)*/
/*
 *	takes a note of the stack-pointer and stores it in the global scheduler structure (accessed through 'p').
 */
static inline void RuntimeSaveStack (Workspace p)
{
	__asm__ __volatile__ ( \
		"	mov	r0, %0						\n" \
		"	ldr	r1, [r0, #0]		@ r1 = p->sched		\n" \
		"	str	sp, [r1, #0]		@ p->sched->stack = sp	\n" \
		: : "r" (p) \
		: "memory", "cc", "r0", "r1", "r2", "r3");
}
/*}}}*/
/*{{{  static inline void RuntimeSetEntry (Workspace p)*/
/* @APICALLCHAIN: RuntimeSetEntry: =8 */
/*
 *	provides an entry-point for processes that can be ProcessResume()'d, simply
 *	branches off to ProcStartupCode (in kfunc.c).
 */
static inline void RuntimeSetEntry (Workspace p)
{
	__asm__ __volatile__ ( \
		"	mov	r0, %0						\n" \
		"	add	r1, r0, #8		@ r1 = &(p->raddr)	\n" \
		"	adr	r2, 0f						\n" \
		"	str	r2, [r1, #0]		@ p->raddr = .Lstart	\n" \
		"	b	1f						\n" \
		"0:								\n" \
		"	add	r3, r0, #4		@ r3 = &(p->stack)	\n" \
		"	ldr	r1, [r3, #0]		@ r1 = p->stack		\n" \
		"	mov	sp, r1			@ switch stack		\n" \
		"	bl	ProcStartupCode(PLT)	@ r0 = process-descriptor	\n" \
		"1:								\n" \
		"								\n" \
		: : "r" (p) \
		: "memory", "cc", "r0", "r1", "r2", "r3");
}
/*}}}*/

#endif	/* !__ARMCCSP_IF_H */

