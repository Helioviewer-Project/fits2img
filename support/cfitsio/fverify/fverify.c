#include <xpi.h>
#include <cftools.h>
#include "fverify.h"
#define  BufLen_2  FLEN_FILENAME-1            /* string parameter maximum length */
/******************************************************************************
* SELECTOR TASK:
*      fverify
*
* FILE:
*      fverify.c
*
* DESCRIPTION:
*      
*     Read a FITS file and verify that it conforms to the FITS standard
*
*
* Usage:
*     fverify infile  outfile
*          infile:  Input fits file.
*          outfile: Output ASCII file. 
*
* AUTHOR:
*      Ning Gan,  24 August 1998 
*
* MODIFICATION HISTORY:
*      1998-08-24  Ning Gan, v3.0.0
*                            Beta version: xverify v1.0
*      1998-12-18  Ning Gan, v3.0.1
*                            Beta version: xverify v1.1
*      1999-02-18  Ning Gan, v3.0.2
*                            Beta version: xverify v1.2 
*                            Added more checks for keywords.
*      1999-03-04  Ning Gan, v3.0.3
*                            Added a feature of multiple input files. 
*      1999-05-19  Ning Gan, v3.0.5
*                            Wrote numwrns and numerrs to the par file.
*                            If the # of errors exceeds the MAXERRORS,
*                            quit and wrote the summary.
*                            Took out the limits on warnings. 
*      1999-06-03  Ning Gan, v3.0.6
*                            Wrote the version number of underlying
*                            cfitsio.
*      1999-06-07  Ning Gan, v3.0.7
*                            Improve the error handling. If there are
*                            errors on opening fitsfile, the program set
*                            numerr to 1 and quit. 
*      1999-06-30  Ning Gan, v3.0.8
*                            Improve the layout of the output.
*      1999-08-25  Ning Gan, v3.0.9
*                            Always write errors to stderr. 
*                            Added ffnchk
*                            Took out the checks of rejecting the
*                            TDISP like I2.0 and the column name
*                            starting with a digit.
*      1999-10-25  Ning Gan, v3.1.0
*                            The TDISP can take format I4.
*                            Beutified the CFISIO error stack output
*                            The numerrs and numwrns are the accumulated
*                            number of errors and warnings if multiple
*                            FITS file are tested in on fverify session.
*                            Checked the X Column is left justified.
*      1999-12-12  Ning Gan, v3.1.1
*                            Added the basiconly and heasarc parameters.
*      1999-12-20  Ning Gan, v3.1.2
*                            Added the parameters of errreport and prstat, 
*                            removed the parameters of basiconly, erronly and 
*                            errpluswrn.
*      1999-12-29  Ning Gan, v3.1.3
*                            fixed a bug on solaris  
*      2000-05-03  Ning Gan, v3.1.4
*                            Skip the blank column names in column name
*                            tests.   
*      2000-06-09  Ning Gan, v3.1.5
*                            Fixed the memory problem(The bug will crash
*      2000-09-26  Ning Gan, v3.1.6
*                            Fixed the TDISP format bug (not accept
*                            format such as E15.5E3).
*      2003-01-09  W Pence   v3.2
*                            Added support for the new WCSAXES keyword
*                            Added support for random groups
*                            several small changes to the output report format
*
*      2004-06-21  W Pence   fixed reporting error when prstat=no and when
*                            opening a nonexistent or non-FITS file.
*                            Also fixed elusive memory allocation error.
*
*      2009-06-08  W Pence   v4.15
*                            updates to comply with V3.0 of the FITS Standard
*      2010-07-26  W Pence   v4.16
*                            Updates to WCS keyword checks, plus other V3.0 issues.
*                            Also check for non-zero heap if binary table has no
*                            variable length array columns.
*      2013-08-12  W Pence   v4.17
*                            Ignore blank keywords preceding the END keyword.
*                            Support (partially at least) files with PCOUNT > 2GB.
*      2016-04-13  B Irby    v4.18
*                            Change verify_fits from void to int; check return
*                            status for abort conditions and set nerrs (as it
*                            is not set by close_report) in this case for the
*                            one-line file summary.
* Reference:
*     * fverify.f, original fortran version, William Pence, 1994.
*     * Defininition of the Flexible Image Transport System(FITS), 
*       NOST 100-1.2, March 29, 1999.
*
*
*******************************************************************************/

/*================= Function prototypes =====================*/
int init_fverify(char *infile, char *outfile, int *status);

/*================= Global variables ========================*/
int err_report= 0;               /* Amounts of errors and warnings to be
                                    reported.
                                    err_report = 0: everything   
                                    err_report = 1: errors only   
                                    err_report = 2: severe errors only.
                                */  
int prhead = 0;			/* print header ? */
int prstat = 1;             /* Print out the prstat? */
int testdata = 1;		/* test data?	*/
int testcsum = 0;		/* test the check sum */
int testfill = 0;		/* test the bytes in the non-data fill areas, 
				   which includes between the 
				   columns of ASCII table, fill areas
				   of the header and data */
int heasarc_conv = 1;		/* Heasarc convention */
int totalhdu = 0;		/* total number of hdu */
static char task[] = "FVERIFY V4.18";

long totalerr, totalwrn;

void fverify()
{ 
    char infile[FLEN_FILENAME] = "";  	/* Input Fits file */
    char outfile[FLEN_FILENAME] = "";  	/* output ASCII file */
    char *p;
    int status = 0, vfstatus = 0, filestatus, ferror, fwarn;
    int i, nerrs; 
    FILE* list = NULL;
    FILE *out;				/* output ASCII file pointer */
    float fversion;
    char *status_str[] = {"OK    ", "FAILED"};

    /*------------------Initialization  --------------------------------*/

    c_ptaskn(task);

    /* get the input parameters from parameter file*/
    if(init_fverify(infile, outfile, &status) ) {
        strcpy(errmes,"Errors in init_fverfify.");
        c_fcerr(errmes);
	leave_early(NULL);
	return;
    } 

    /* if infile name has @ in front of it, then it is a file of a list */ 
    p = infile; 
    if (*p == '@') { 
         p++;
	 if( (list = fopen(p,"r")) == NULL ) { 
	        sprintf(errmes,
		    "Can not open the list file: %s.",p);
                c_fcerr(errmes);
	        leave_early(NULL);
		return;
         } 
    }     

    /* if the outfile is given, open it, otherwise, use stdout */
    p = outfile;
    if(*p == '!') { 
	p++;
    } 
    else { 
        if(prstat && strlen(p) && strcmp(p, "STDOUT")) { 
	    if( (out = fopen(p,"r")) != NULL ) { 
	        sprintf(errmes,
		    "Clobber is not set. Can not overwrite the file %s.",p);
                c_fcerr(errmes);
	        leave_early(NULL);
		fclose(out);
		return;
            }
        }
    }

    if(prstat && (!strlen(p) || !strcmp(p, "STDOUT"))) {  
	out = stdout;
    }
    else if (!prstat) {
       out=NULL;
    }  
    else if( (out = fopen(p,"w") ) == NULL) {
            sprintf(errmes,
               "Error opening Ouputput file %s. Using stdout instead.",
	       outfile);
            c_fcerr(errmes);
	    out = stdout;
    }

    wrtout(out," ");
    fits_get_version(&fversion);
    sprintf(comm,"%s (CFITSIO V%.3f)",task,fversion);
    wrtsep(out,' ',comm,75);
    for(i = 0; comm[i]!='\0'; i++) comm[i] = '-';
    wrtsep(out,' ',comm,75);
    wrtout(out," ");
    switch (err_report) {
    case 2:
    sprintf(comm, "Caution: Only checking for the most severe FITS format errors.");
        wrtout(out,comm);
        break;
    case 1:
        break;
    case 0:
        break;
    }

    if(heasarc_conv) {
        sprintf(comm, "HEASARC conventions are being checked.");
	wrtout(out,comm);
    }     

    /* process each file */ 
    if (list == NULL) { 
        vfstatus = verify_fits(infile,out);  

        if (out == NULL) {  /* print one-line file summary */

           /* verify_fits returns a non-zero status for catastrophic
            * file I/O problems (an abort), and in this case total_err
            * is not updated via close_report(), so we need to set
            * nerrs accordingly for the one-line file summary. */
           if (vfstatus) nerrs = 1; else nerrs = get_total_err();

           filestatus = (nerrs>0) ? 1 : 0;
           printf("verification %s: %-20s  nerrors=%d\n", status_str[filestatus],
                   infile, nerrs); 
        }        
    } 
    else { 
       while((p = fgets(infile, FLEN_FILENAME, list))!= NULL) {      
           vfstatus = verify_fits(infile,out);   

           if (out == NULL) { /* print one-line file summary */

              /* verify_fits returns a non-zero status for catastrophic
               * file I/O problems (an abort), and in this case total_err
               * is not updated via close_report(), so we need to set
               * nerrs accordingly for the one-line file summary. */
              if (vfstatus) nerrs = 1; else nerrs = get_total_err();

              filestatus = (nerrs>0) ? 1 : 0;
              printf("verification %s: %-20s  nerrors=%d\n",
                 status_str[filestatus], infile, nerrs); 
           }        

           for (i = 1; i < 3; i++) wrtout(out," ");
       }
       fclose(list);
    }        
 
    /* close the output file  */ 
    if (out != stdout && out != NULL) fclose(out);

    return;
}    
        
/******************************************************************************
* Function
*      init_fverify
*
* DESCRIPTION:
*      Get parameters from the par file.
*
*******************************************************************************/
int init_fverify(char *infile, 		/* input filename+filter */
		char *outfile, 		/* output filename */
		int *status
		)
{
    return 0;
}
    
/******************************************************************************
* Function
*      update_parfile
*
*
* DESCRIPTION:
*      Update the numerrs and numwrns parameters in the parfile.
*
*******************************************************************************/
    void update_parfile(int nerr, int nwrn)
{
}
