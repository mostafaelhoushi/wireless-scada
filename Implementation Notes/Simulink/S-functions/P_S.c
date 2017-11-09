/*
* P_S.c: Implements a p-controller.
*
*  -------------------------------------------------------------------------
*  | See matlabroot/simulink/src/sfuntmpl_doc.c for a more detailed template 
|
*  -------------------------------------------------------------------------
*
*/


/*
* You must specify the S_FUNCTION_NAME as the name of your S-function
* (i.e. replace sfuntmpl_basic with the name of your S-function).
*/

#define S_FUNCTION_NAME P_S

#define S_FUNCTION_LEVEL 2

/*#define BASE_ADDRESS_PRM(S) ssGetFunParam(S,0)
#define GAIN_RANGE_PRM(S) ssGetFunParam(S,1)
#define PROG_GAIN_PRM(S) ssGetFunParam(S,2)
#define NUM_OF_CHANNELS_PRM(S) ssGetFunParam(S,3)*/

/*
* Need to include simstruc.h for the definition of the SimStruct and
* its associated macro definitions.
*/
#include "simstruc.h"
     

#include <math.h>
#include <stdio.h>
#include <time.h>

#define NPARAMS              2

/* Parameter  1 */
#define PARAMETER_0_NAME      Ts
#define PARAMETER_0_DTYPE     real_T
#define PARAMETER_0_COMPLEX   COMPLEX_NO

#define SAMPLE_TIME_0        INHERITED_SAMPLE_TIME
#define NUM_DISC_STATES      0
#define DISC_STATES_IC       [0]
#define NUM_CONT_STATES      0
#define CONT_STATES_IC       [0]

#define SFUNWIZ_GENERATE_TLC 1
#define SOURCEFILES "__SFB__"
#define PANELINDEX           6
#define SFUNWIZ_REVISION     3.0
#define FUNCTION __declspec(dllexport)

/* Parameter  2 */                                                         //////////////////==
#define PARAMETER_0_NAME      Kp
#define PARAMETER_0_DTYPE     real_T
#define PARAMETER_0_COMPLEX   COMPLEX_NO

#define SAMPLE_TIME_0        INHERITED_SAMPLE_TIME
#define NUM_DISC_STATES      0
#define DISC_STATES_IC       [0]
#define NUM_CONT_STATES      0
#define CONT_STATES_IC       [0]

#define SFUNWIZ_GENERATE_TLC 1
#define SOURCEFILES "__SFB__"
#define PANELINDEX           6
#define SFUNWIZ_REVISION     3.0
#define FUNCTION __declspec(dllexport)



/* Error handling
* --------------
*
* You should use the following technique to report errors encountered within
* an S-function:
*
*       ssSetErrorStatus(S,"Error encountered due to ...");
*       return;
*
* Note that the 2nd argument to ssSetErrorStatus must be persistent memory.
* It cannot be a local variable. For example the following will cause
* unpredictable errors:
*
*      mdlOutputs()
*      {
*         char msg[256];         {ILLEGAL: to fix use "static char 
msg[256];"}
*         sprintf(msg,"Error due to %s", string);
*         ssSetErrorStatus(S,msg);
*         return;
*      }
*
* See matlabroot/simulink/src/sfuntmpl_doc.c for more details.
*/
#define PARAM_DEF0(S) ssGetSFcnParam(S, 0)
#define PARAM_DEF1(S) ssGetSFcnParam(S, 1)

#define IS_PARAM_DOUBLE(pVal) (mxIsNumeric(pVal) && !mxIsLogical(pVal) &&\
!mxIsEmpty(pVal) && !mxIsSparse(pVal) && !mxIsComplex(pVal) && mxIsDouble(pVal))

/*====================*
* S-function methods *
*====================*/
  int h = -1;

//#define MDL_CHECK_PARAMETERS
 //#if defined(MDL_CHECK_PARAMETERS) && defined(MATLAB_MEX_FILE)
 
/* Function: mdlInitializeSizes 
===============================================
* Abstract:
*    The sizes information is used by Simulink to determine the S-function
*    block's characteristics (number of inputs, outputs, states, etc.).
*/
//static void mdlCheckParameters(SimStruct *S)
 //   {
   /*  #define PrmNumPos 46
     int paramIndex = 0;
     bool validParam = false;
     char paramVector[] ={'1'};
     static char parameterErrorMsg[] ="The data type and/or complexity of parameter    does not match the information "
     "specified in the S-function Builder dialog. For non-double parameters you will need to cast them using int8, int16,"
     "int32, uint8, uint16, uint32 or boolean."; 

     /* All parameters must match the S-function Builder Dialog */
     

	// {
/*	  const mxArray *pVal0 = ssGetSFcnParam(S,0);
	  if (!IS_PARAM_DOUBLE(pVal0)) {
	    validParam = true;
	    paramIndex = 0;
	    goto EXIT_POINT;
	  }
	 }
     EXIT_POINT:
      if (validParam) {
	  parameterErrorMsg[PrmNumPos] = paramVector[paramIndex];
	  ssSetErrorStatus(S,parameterErrorMsg);
      }
	return;
    }
 
 #endif /* MDL_CHECK_PARAMETERS */

         static void mdlInitializeSizes(SimStruct *S)
{
    /* See sfuntmpl_doc.c for more details on the macros below */

    int_T nOutputPorts = 1;  /* number of output ports */

   ssSetNumSFcnParams(S, NPARAMS);  /* Number of expected parameters */
   // #if defined(MATLAB_MEX_FILE)
    if (ssGetNumSFcnParams(S) != ssGetSFcnParamsCount(S)) {
        /* Return if number of expected != number of actual parameters */
      //   mdlCheckParameters(S);
     //  if (ssGetErrorStatus(S) != NULL) {
	    return;
	  }

//	 } else {
//	   return; /* Parameter mismatch will be reported by Simulink */
         
  //  }
    //  #endif
    
    ssSetNumContStates(S, 0);
    ssSetNumDiscStates(S, 0);

    if (!ssSetNumInputPorts(S, 1)) return;
     ssSetInputPortWidth(S, 0, DYNAMICALLY_SIZED);

    ssSetInputPortRequiredContiguous(S, 0, true); /*direct input signal 
access*/
    /*
     * Set direct feedthrough flag (1=yes, 0=no).
     * A port has direct feedthrough if the input is used in either
     * the mdlOutputs or mdlGetTimeOfNextVarHit functions.
     * See matlabroot/simulink/src/sfuntmpl_directfeed.txt.
     */
    ssSetInputPortDirectFeedThrough(S, 0, 1);

    if (!ssSetNumOutputPorts(S,1)) return;
   ssSetOutputPortWidth(S, 0, DYNAMICALLY_SIZED);
   // ssSetOutputPortWidth(S, 1, DYNAMICALLY_SIZED);

    ssSetNumSampleTimes(S, 1);
    ssSetNumRWork(S, 0);
    ssSetNumIWork(S, 0);
    ssSetNumPWork(S, 0);
    ssSetNumModes(S, 0);
    ssSetNumNonsampledZCs(S, 0);

    ssSetOptions(S, 0);
}



/* Function: mdlInitializeSampleTimes 
=========================================
* Abstract:
*    This function is used to specify the sample time(s) for your
*    S-function. You must register the same number of sample times as
*    specified in ssSetNumSampleTimes.
*/

static void mdlInitializeSampleTimes(SimStruct *S)
{
      real_T  *Ts  = mxGetData(PARAM_DEF0(S)); 
   
   
     ssSetSampleTime(S, 0, Ts[0]);
    
    ssSetOffsetTime(S, 0, 0.0);

}

 
#define MDL_INITIALIZE_CONDITIONS   /* Change to #undef to remove function 
*/
#if defined(MDL_INITIALIZE_CONDITIONS)
  /* Function: mdlInitializeConditions 
========================================
   * Abstract:
   *    In this function, you should initialize the continuous and discrete
   *    states for your S-function block.  The initial states are placed
   *    in the state vector, ssGetContStates(S) or ssGetRealDiscStates(S).
   *    You can also perform any other initialization activities that your
   *    S-function may require. Note, this routine will be called at the
   *    start of simulation and if it is present in an enabled subsystem
   *    configured to reset states, it will be call when the enabled 
subsystem
   *    restarts execution to reset the states.
   */
  static void mdlInitializeConditions(SimStruct *S)
  {
       int i;
  real_T *xcont    = ssGetContStates(S);
  int_T   nCStates = ssGetNumContStates(S);
  real_T *xdisc    = ssGetRealDiscStates(S); 
  int_T   nDStates = ssGetNumDiscStates(S);

  for (i = 0; i < nCStates; i++) {
    *xcont++ = 1.0;
  }

  for (i = 0; i < nDStates; i++) {
    *xdisc++ = 1.0;
  }


  }
#endif /* MDL_INITIALIZE_CONDITIONS */



#define MDL_START  /* Change to #undef to remove function */
#if defined(MDL_START)
  /* Function: mdlStart 
=======================================================
   * Abstract:
   *    This function is called once at start of model execution. If you
   *    have states that should be initialized once, this is the place
   *    to do it.
   */
  static void mdlStart(SimStruct *S)
  {
////////////////////////////////////////////////////////initilize here////////////////////////   

   
  }
#endif /*  MDL_START */



/* Function: mdlOutputs 
=======================================================
* Abstract:
*    In this function, you compute the outputs of your S-function
*    block. Generally outputs are placed in the output vector, ssGetY(S).
*/


static void mdlOutputs(SimStruct *S, int_T tid)
{    
   double e , m;
   real_T  *Ts  = mxGetData(PARAM_DEF0(S)); 
   real_T  *Kp  = mxGetData(PARAM_DEF1(S)); 
    
   const real_T *u = (const real_T*) ssGetInputPortSignal(S,0);
   real_T       *y = ssGetOutputPortSignal(S,0);
    
   e = u[0];
   
   m = *Kp * e;
   
   y[0]=m;

}


#define MDL_UPDATE  /* Change to #undef to remove function */
#if defined(MDL_UPDATE)
  /* Function: mdlUpdate 
======================================================
   * Abstract:
   *    This function is called once for every major integration time step.
   *    Discrete states are typically updated here, but this function is 
useful
   *    for performing any tasks that should only take place once per
   *    integration step.
   */
  static void mdlUpdate(SimStruct *S, int_T tid)
  {
  }
#endif /* MDL_UPDATE */



#define MDL_DERIVATIVES  /* Change to #undef to remove function */
#if defined(MDL_DERIVATIVES)
  /* Function: mdlDerivatives 
=================================================
   * Abstract:
   *    In this function, you compute the S-function block's derivatives.
   *    The derivatives are placed in the derivative vector, ssGetdX(S).
   */
  static void mdlDerivatives(SimStruct *S)
  {
  }
#endif /* MDL_DERIVATIVES */



/* Function: mdlTerminate 
=====================================================
* Abstract:
*    In this function, you should perform any actions that are necessary
*    at the termination of a simulation.  For example, if memory was
*    allocated in mdlStart, this is the place to free it.
*/
static void mdlTerminate(SimStruct *S)
{

}


/*======================================================*
* See sfuntmpl_doc.c for the optional S-function methods *
*======================================================*/

/*=============================*
* Required S-function trailer *
*=============================*/

#ifdef  MATLAB_MEX_FILE    /* Is this file being compiled as a MEX-file? */
#include "simulink.c"      /* MEX-file interface mechanism */
#else
#include "cg_sfun.h"       /* Code generation registration function */
#endif