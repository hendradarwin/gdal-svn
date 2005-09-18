/*
 * $Id$
 *
 * Code for Optional Exception Handling through UseExceptions(),
 * DontUseExceptions()
 *
 * It uses CPLSetErrorHandler to provide a custom function
 * which notifies the bindings of errors. 
 *
 * This is not thread safe.
 *
 * $Log$
 * Revision 1.2  2005/09/18 07:36:18  cfis
 * Only raise exceptions on failures or fatal errors.  The previous code rose exceptions on debug messages, warning messages and when nothing at all happened.
 *
 * Revision 1.1  2005/09/13 03:04:27  kruland
 * Pull the exception generation mechanism out of gdal_python.i so it could
 * be used by other bindings.
 *
 *
 */

%{
int bUseExceptions=0;
int bErrorHappened=0;

void ErrorHandler(CPLErr eclass, int code, const char *msg ) {
  /* Only raise exceptions on failures and fatal errors */
  if (eclass == CE_Failure || eclass == CE_Fatal) {
    bErrorHappened = 1;
  }
}
%}

%inline %{
void UseExceptions() {
  bUseExceptions = 1;
  bErrorHappened = 0;
  CPLSetErrorHandler( (CPLErrorHandler)ErrorHandler );
}

void DontUseExceptions() {
  bUseExceptions = 0;
  bErrorHappened = 0;
  CPLSetErrorHandler( CPLDefaultErrorHandler );
}
%}

%include exception.i

%exception {
  {
    bErrorHappened = 0;
    $action
    if ( bErrorHappened ) {
      SWIG_exception( SWIG_RuntimeError, CPLGetLastErrorMsg() );
    }
  }
}
