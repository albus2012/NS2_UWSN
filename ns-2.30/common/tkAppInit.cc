/* 
 * tkAppInit.c --
 *
 *	Provides a default version of the Tcl_AppInit procedure for
 *	use in wish and similar Tk-based applications.
 *
 * Copyright (c) 1993 The Regents of the University of California.
 * Copyright (c) 1994 Sun Microsystems, Inc.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * SCCS: @(#) tkAppInit.c 1.21 96/03/26 16:47:07
 */

#include <tk.h>
#include "config.h"

extern init_misc();

extern "C" {

#ifdef TK_TEST
EXTERN int		Tktest_Init _ANSI_ARGS_((Tcl_Interp *interp));
#endif /* TK_TEST */

/*
 *----------------------------------------------------------------------
 *
 * main --
 *
 *	This is the main program for the application.
 *
 * Results:
 *	None: Tk_Main never returns here, so this procedure never
 *	returns either.
 *
 * Side effects:
 *	Whatever the application does.
 *
 *----------------------------------------------------------------------
 */

int
main(int argc, char** argv)
{
    Tk_Main(argc, argv, Tcl_AppInit);
    return 0;			/* Needed only to prevent compiler warning. */
}


#include "bitmap/play.xbm"
#include "bitmap/stop.xbm"
#include "bitmap/rewind.xbm"
#include "bitmap/ff.xbm"

void loadbitmaps(Tcl_Interp* tcl)
{
	Tk_DefineBitmap(tcl, Tk_GetUid("play"),
			play_bits, play_width, play_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("stop"),
			stop_bits, stop_width, stop_height);

	Tk_DefineBitmap(tcl, Tk_GetUid("rewind"),
			rewind_bits, rewind_width, rewind_height);
	Tk_DefineBitmap(tcl, Tk_GetUid("ff"),
			ff_bits, ff_width, ff_height);
}



/*
 *----------------------------------------------------------------------
 *
 * Tcl_AppInit --
 *
 *	This procedure performs application-specific initialization.
 *	Most applications, especially those that incorporate additional
 *	packages, will have their own version of this procedure.
 *
 * Results:
 *	Returns a standard Tcl completion code, and leaves an error
 *	message in interp->result if an error occurs.
 *
 * Side effects:
 *	Depends on the startup script.
 *
 *----------------------------------------------------------------------
 */

int
Tcl_AppInit(Tcl_Interp *interp)
{
    if (Tcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Otcl_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    if (Tk_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }

    Tcl_StaticPackage(interp, "Tk", Tk_Init, (Tcl_PackageInitProc *) NULL);
#ifdef TK_TEST
    if (Tktest_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
    Tcl_StaticPackage(interp, "Tktest", Tktest_Init,
            (Tcl_PackageInitProc *) NULL);
#endif /* TK_TEST */


    /*
     * Call the init procedures for included packages.  Each call should
     * look like this:
     *
     * if (Mod_Init(interp) == TCL_ERROR) {
     *     return TCL_ERROR;
     * }
     *
     * where "Mod" is the name of the module.
     */
#ifdef HAVE_LIBTCLDBG
    if (Dbg_Init(interp) == TCL_ERROR) {
	return TCL_ERROR;
    }
#endif

    Tcl::init(interp, "ns");
    Tcl::instance().tkmain(Tk_MainWindow(interp));
    extern EmbeddedTcl et_ns_lib, et_tk;
    et_tk.load();
    et_ns_lib.load();
    init_misc();
    loadbitmaps(interp);


    /*
     * Call Tcl_CreateCommand for application-specific commands, if
     * they weren't already created by the init procedures called above.
     */

    /*
     * Specify a user-specific startup file to invoke if the application
     * is run interactively.  Typically the startup file is "~/.apprc"
     * where "app" is the name of the application.  If this line is deleted
     * then no user-specific startup file will be run under any conditions.
     */

    Tcl_SetVar(interp, "tcl_rcFileName", "~/.ns.tcl", TCL_GLOBAL_ONLY);

    return TCL_OK;
}

abort()
{
	Tcl& tcl = Tcl::instance();
	tcl.evalc("[Simulator instance] flush-trace");
#ifdef abort
#undef abort
	abort();
#else
	exit(1);
#endif /*abort*/
	/*NOTREACHED*/
}

}
