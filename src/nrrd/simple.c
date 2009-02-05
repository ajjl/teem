/*
  Teem: Tools to process and visualize scientific data and images              
  Copyright (C) 2008, 2007, 2006, 2005  Gordon Kindlmann
  Copyright (C) 2004, 2003, 2002, 2001, 2000, 1999, 1998  University of Utah

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public License
  (LGPL) as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.
  The terms of redistributing and/or modifying this software also
  include exceptions to the LGPL that facilitate static linking.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "nrrd.h"
#include "privateNrrd.h"

#include <limits.h>

const char *
nrrdBiffKey = "nrrd";

/*
******** nrrdSpaceDimension
**
** returns expected dimension of given space (from nrrdSpace* enum), or,
** 0 if there is no expected dimension.
**
** The expected behavior here is to return 0 for nrrdSpaceUnknown, because
** that is the right answer, not because its an error of any kind.
*/
unsigned int
nrrdSpaceDimension(int space) {
  char me[]="nrrdSpaceDimension";
  int ret;

  if (!( AIR_IN_OP(nrrdSpaceUnknown, space, nrrdSpaceLast) )) {
    /* they gave us invalid or unknown space */
    return 0;
  }
  switch (space) {
  case nrrdSpaceRightAnteriorSuperior:
  case nrrdSpaceLeftAnteriorSuperior:
  case nrrdSpaceLeftPosteriorSuperior:
  case nrrdSpaceScannerXYZ:
  case nrrdSpace3DRightHanded:
  case nrrdSpace3DLeftHanded:
    ret = 3;
    break;
  case nrrdSpaceRightAnteriorSuperiorTime:
  case nrrdSpaceLeftAnteriorSuperiorTime:
  case nrrdSpaceLeftPosteriorSuperiorTime:
  case nrrdSpaceScannerXYZTime:
  case nrrdSpace3DRightHandedTime:
  case nrrdSpace3DLeftHandedTime:
    ret = 4;
    break;
  default:
    fprintf(stderr, "%s: PANIC: nrrdSpace %d not implemented!\n", me, space);
    exit(1);
    break;
  }
  return ret;
}

/*
******** nrrdSpaceSet
**
** What to use to set space, when a value from nrrdSpace enum is known,
** or, to nullify all space-related information when passed nrrdSpaceUnknown
*/
int
nrrdSpaceSet(Nrrd *nrrd, int space) {
  char me[]="nrrdSpaceSet", err[BIFF_STRLEN];
  unsigned axi, saxi;
  
  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdSpaceUnknown == space) {
    nrrd->space = nrrdSpaceUnknown;
    nrrd->spaceDim = 0;
    for (axi=0; axi<NRRD_DIM_MAX; axi++) {
      nrrdSpaceVecSetNaN(nrrd->axis[axi].spaceDirection);
    }
    for (saxi=0; saxi<NRRD_SPACE_DIM_MAX; saxi++) {
      airFree(nrrd->spaceUnits[saxi]);
      nrrd->spaceUnits[saxi] = NULL;
    }
    nrrdSpaceVecSetNaN(nrrd->spaceOrigin);
  } else {
    if (airEnumValCheck(nrrdSpace, space)) {
      sprintf(err, "%s: given space (%d) not valid", me, space);
      biffAdd(NRRD, err); return 1;
    }
    nrrd->space = space;
    nrrd->spaceDim = nrrdSpaceDimension(space);
  }
  return 0;
}

/*
******** nrrdSpaceDimensionSet
**
** What to use to set space, based on spaceDim alone (nrrd->space set to
** nrrdSpaceUnknown)
*/
int
nrrdSpaceDimensionSet(Nrrd *nrrd, unsigned int spaceDim) {
  char me[]="nrrdSpaceDimensionSet", err[BIFF_STRLEN];
  
  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( spaceDim <= NRRD_SPACE_DIM_MAX )) {
    sprintf(err, "%s: given spaceDim (%u) not valid", me, spaceDim);
    biffAdd(NRRD, err); return 1;
  }
  nrrd->space = nrrdSpaceUnknown;
  nrrd->spaceDim = spaceDim;
  return 0;
}

/*
******** nrrdSpaceOriginGet
**
** retrieves the spaceOrigin from given nrrd, and returns spaceDim
** Indices 0 through spaceDim-1 are set in given vector[] to coords
** of space origin, and all further indices are set to NaN
*/
unsigned int
nrrdSpaceOriginGet(const Nrrd *nrrd,
                   double vector[NRRD_SPACE_DIM_MAX]) {
  unsigned int sdi, ret;

  if (nrrd && vector) {
    for (sdi=0; sdi<nrrd->spaceDim; sdi++) {
      vector[sdi] = nrrd->spaceOrigin[sdi];
    }
    for (sdi=nrrd->spaceDim; sdi<NRRD_SPACE_DIM_MAX; sdi++) {
      vector[sdi] = AIR_NAN;
    }
    ret = nrrd->spaceDim;
  } else {
    ret = 0;
  }
  return ret;
}

/*
******** nrrdSpaceOriginSet
**
** convenience function for setting spaceOrigin.
** Note: space (or spaceDim) must be already set
**
** returns 1 if there were problems, 0 otherwise
*/
int
nrrdSpaceOriginSet(Nrrd *nrrd,
                   double vector[NRRD_SPACE_DIM_MAX]) {
  char me[]="nrrdSpaceOriginSet", err[BIFF_STRLEN];
  unsigned int sdi;

  if (!( nrrd && vector )) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (!( 0 < nrrd->spaceDim && nrrd->spaceDim <= NRRD_SPACE_DIM_MAX )) {
    sprintf(err, "%s: set spaceDim %d not valid", me, nrrd->spaceDim);
    biffAdd(NRRD, err); return 1;
  }

  for (sdi=0; sdi<nrrd->spaceDim; sdi++) {
    nrrd->spaceOrigin[sdi] = vector[sdi];
  }
  for (sdi=nrrd->spaceDim; sdi<NRRD_SPACE_DIM_MAX; sdi++) {
    nrrd->spaceOrigin[sdi] = AIR_NAN;
  }
  return 0;
}

/*
******** nrrdOriginCalculate
**
** makes an effort to calculate something like an "origin" (as in
** nrrd->spaceOrigin) from the per-axis min, max, or spacing, when
** there is no real space information.  Like the spaceOrigin, this
** location is supposed to be THE CENTER of the first sample.  To
** avoid making assumptions about the nrrd or the caller, a default
** sample centering (defaultCenter) has to be provided (use either
** nrrdCenterNode or nrrdCenterCell).  The axes that are used 
** for the origin calculation have to be given explicitly- but they
** are likely the return of nrrdDomainAxesGet
**
** The computed origin is put into the given vector (origin).  The return
** value takes on values from the nrrdOriginStatus* enum:
**
** nrrdOriginStatusUnknown:        invalid arguments (e.g. NULL pointer, or 
**                                 axis values out of range)
**
** nrrdOriginStatusDirection:      the chosen axes have spaceDirection set, 
**                                 which means caller should instead be using
**                                 nrrdSpaceOriginGet
**
** nrrdOriginStatusNoMin:          can't compute "origin" without axis->min
**
** nrrdOriginStatusNoMaxOrSpacing: can't compute origin without (axis->min
**                                 and) either axis->max or axis->spacing
**
** nrrdOriginStatusOkay:           all is well
*/
int
nrrdOriginCalculate(const Nrrd *nrrd, 
                    unsigned int *axisIdx, unsigned int axisIdxNum,
                    int defaultCenter, double *origin) {
  const NrrdAxisInfo *axis[NRRD_SPACE_DIM_MAX];
  int center, okay, gotSpace, gotMin, gotMaxOrSpacing;
  unsigned int ai;
  double min, spacing;

#define ERROR \
  if (origin) { \
    for (ai=0; ai<axisIdxNum; ai++) { \
      origin[ai] = AIR_NAN; \
    } \
  }

  if (!( nrrd 
         && (nrrdCenterCell == defaultCenter
             || nrrdCenterNode == defaultCenter)
         && origin )) {
    ERROR;
    return nrrdOriginStatusUnknown;
  }

  okay = AIR_TRUE;
  for (ai=0; ai<axisIdxNum; ai++) {
    okay &= axisIdx[ai] < nrrd->dim;
  }
  if (!okay) {
    ERROR;
    return nrrdOriginStatusUnknown;
  }

  /* learn axisInfo pointers */
  for (ai=0; ai<axisIdxNum; ai++) {
    axis[ai] = nrrd->axis + axisIdx[ai];
  }

  gotSpace = AIR_FALSE;
  for (ai=0; ai<axisIdxNum; ai++) {
    gotSpace |= AIR_EXISTS(axis[ai]->spaceDirection[0]);
  }
  if (nrrd->spaceDim > 0 && gotSpace) {
    ERROR;
    return nrrdOriginStatusDirection;
  }

  gotMin = AIR_TRUE;
  for (ai=0; ai<axisIdxNum; ai++) {
    gotMin &= AIR_EXISTS(axis[0]->min);
  }
  if (!gotMin) {
    ERROR;
    return nrrdOriginStatusNoMin;
  }

  gotMaxOrSpacing = AIR_TRUE;
  for (ai=0; ai<axisIdxNum; ai++) {
    gotMaxOrSpacing &= (AIR_EXISTS(axis[ai]->max)
                        || AIR_EXISTS(axis[ai]->spacing));
  }
  if (!gotMaxOrSpacing) {
    ERROR;
    return nrrdOriginStatusNoMaxOrSpacing;
  }
  
  for (ai=0; ai<axisIdxNum; ai++) {
    size_t size;
    size = axis[ai]->size;
    min = axis[ai]->min;
    center = (nrrdCenterUnknown != axis[ai]->center
              ? axis[ai]->center
              : defaultCenter);
    spacing = (AIR_EXISTS(axis[ai]->spacing)
               ? axis[ai]->spacing
               : ((axis[ai]->max - min)
                  /(nrrdCenterCell == center ? size : size-1)));
    origin[ai] = min + (nrrdCenterCell == center ? spacing/2 : 0);
  }
  return nrrdOriginStatusOkay;
}

void
nrrdSpaceVecCopy(double dst[NRRD_SPACE_DIM_MAX], 
                  const double src[NRRD_SPACE_DIM_MAX]) {
  int ii;

  for (ii=0; ii<NRRD_SPACE_DIM_MAX; ii++) {
    dst[ii] = src[ii];
  }
}

/*
** NOTE: since this was created until Wed Sep 21 13:34:17 EDT 2005,
** nrrdSpaceVecScaleAdd2 and nrrdSpaceVecScale would treat a
** non-existent vector coefficient as 0.0.  The reason for this had
** to do with how the function is used.  For example, in nrrdCrop
**
**   _nrrdSpaceVecCopy(nout->spaceOrigin, nin->spaceOrigin);
**   for (ai=0; ai<nin->dim; ai++) {
**      _nrrdSpaceVecScaleAdd2(nout->spaceOrigin,
**                             1.0, nout->spaceOrigin,
**                             min[ai], nin->axis[ai].spaceDirection);
**   }
**
** but the problem with this is that non-spatial axes end up clobbering
** the existance of otherwise existing spaceOrigin and spaceDirections.
** It was decided, however, that this logic should be outside the
** arithmetic functions below, not inside.  NOTE: the old functionality
** is stuck in ITK 2.2, via NrrdIO.
*/

void
nrrdSpaceVecScaleAdd2(double sum[NRRD_SPACE_DIM_MAX], 
                       double sclA, const double vecA[NRRD_SPACE_DIM_MAX],
                       double sclB, const double vecB[NRRD_SPACE_DIM_MAX]) {
  int ii;
  
  for (ii=0; ii<NRRD_SPACE_DIM_MAX; ii++) {
    sum[ii] = sclA*vecA[ii] + sclB*vecB[ii];
  }
}

void
nrrdSpaceVecScale(double out[NRRD_SPACE_DIM_MAX], 
                   double scl, const double vec[NRRD_SPACE_DIM_MAX]) {
  int ii;
  
  for (ii=0; ii<NRRD_SPACE_DIM_MAX; ii++) {
    out[ii] = scl*vec[ii];
  }
}

double
nrrdSpaceVecNorm(int sdim, const double vec[NRRD_SPACE_DIM_MAX]) {
  int di;
  double nn;

  nn = 0;
  for (di=0; di<sdim; di++) {
    nn += vec[di]*vec[di];
  }
  return sqrt(nn);
}

void
nrrdSpaceVecSetNaN(double vec[NRRD_SPACE_DIM_MAX]) {
  int di;

  for (di=0; di<NRRD_SPACE_DIM_MAX; di++) {
    vec[di] = AIR_NAN;
  }
  return;
}

/*
** _nrrdContentGet
**
** ALLOCATES a string for the content of a given nrrd
** panics and exits if allocation failed
*/
char *
_nrrdContentGet(const Nrrd *nin) {
  char me[]="_nrrdContentGet";
  char *ret;
  
  ret = ((nin && nin->content) ? 
         airStrdup(nin->content) : 
         airStrdup(nrrdStateUnknownContent));
  if (!ret) {
    fprintf(stderr, "%s: PANIC: content strdup failed!\n", me);
    exit(1);
  }
  return ret;
}

int
_nrrdContentSet_nva(Nrrd *nout, const char *func,
                    char *content, const char *format, va_list arg) {
  char me[]="_nrrdContentSet_nva", err[BIFF_STRLEN], *buff;

  buff = (char *)malloc(128*AIR_STRLEN_HUGE);
  if (!buff) {
    sprintf(err, "%s: couln't alloc buffer!", me);
    biffAdd(NRRD, err); return 1;
  }
  nout->content = (char *)airFree(nout->content);

  /* we are currently praying that this won't overflow the "buff" array */
  /* HEY: replace with vsnprintf or whatever when its available */
  vsprintf(buff, format, arg);

  nout->content = (char *)calloc(strlen("(,)")
                                 + airStrlen(func)
                                 + 1                      /* '(' */
                                 + airStrlen(content)
                                 + 1                      /* ',' */
                                 + airStrlen(buff)
                                 + 1                      /* ')' */
                                 + 1, sizeof(char));      /* '\0' */
  if (!nout->content) {
    sprintf(err, "%s: couln't alloc output content!", me);
    biffAdd(NRRD, err); airFree(buff); return 1;
  }
  sprintf(nout->content, "%s(%s%s%s)", func, content,
          airStrlen(buff) ? "," : "", buff);
  airFree(buff);  /* no NULL assignment, else compile warnings */
  return 0;
}

int
_nrrdContentSet_va(Nrrd *nout, const char *func,
                   char *content, const char *format, ...) {
  char me[]="_nrrdContentSet_va", err[BIFF_STRLEN];
  va_list ap;
  
  va_start(ap, format);
  if (_nrrdContentSet_nva(nout, func, content, format, ap)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); free(content); return 1;
  }
  va_end(ap);

  /* free(content);  */
  return 0;
}

/*
******** nrrdContentSet
**
** Kind of like sprintf, but for the content string of the nrrd.
**
** Whether or not we write a new content for an old nrrd ("nin") with
** NULL content is decided here, according to
** nrrdStateAlwaysSetContent.
**
** Does the string allocation and some attempts at error detection.
** Does allow nout==nin, which requires some care.
*/
int
nrrdContentSet_va(Nrrd *nout, const char *func,
                  const Nrrd *nin, const char *format, ...) {
  char me[]="nrrdContentSet_va", err[BIFF_STRLEN];
  va_list ap;
  char *content;
  
  if (!(nout && func && nin && format)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffAdd(NRRD, err); return 1;
  }
  if (nrrdStateDisableContent) {
    /* we kill content always */
    nout->content = (char *)airFree(nout->content);
    return 0;
  }
  if (!nin->content && !nrrdStateAlwaysSetContent) {
    /* there's no input content, and we're not supposed to invent any
       content, so after freeing nout's content we're done */
    nout->content = (char *)airFree(nout->content);
    return 0;
  }
  /* we copy the input nrrd content first, before blowing away the
     output content, in case nout == nin */
  content = _nrrdContentGet(nin);
  va_start(ap, format);
  if (_nrrdContentSet_nva(nout, func, content, format, ap)) {
    sprintf(err, "%s:", me);
    biffAdd(NRRD, err); va_end(ap); free(content); return 1;
  }
  va_end(ap);
  free(content); 

  return 0;
}

/*
******** nrrdDescribe
** 
** writes verbose description of nrrd to given file
*/
void
nrrdDescribe(FILE *file, const Nrrd *nrrd) {
  unsigned int ai;

  if (file && nrrd) {
    fprintf(file, "Nrrd at 0x%p:\n", (void*)nrrd);
    fprintf(file, "Data at 0x%p is " _AIR_SIZE_T_CNV
            " elements of type %s.\n",
            nrrd->data, nrrdElementNumber(nrrd), 
            airEnumStr(nrrdType, nrrd->type));
    if (nrrdTypeBlock == nrrd->type) {
      fprintf(file, "The blocks have size " _AIR_SIZE_T_CNV "\n",
              nrrd->blockSize);
    }
    if (airStrlen(nrrd->content)) {
      fprintf(file, "Content = \"%s\"\n", nrrd->content);
    }
    fprintf(file, "%d-dimensional array, with axes:\n", nrrd->dim);
    for (ai=0; ai<nrrd->dim; ai++) {
      if (airStrlen(nrrd->axis[ai].label)) {
        fprintf(file, "%d: (\"%s\") ", ai, nrrd->axis[ai].label);
      } else {
        fprintf(file, "%d: ", ai);
      }
      fprintf(file, "%s-centered, size=" _AIR_SIZE_T_CNV ", ",
              airEnumStr(nrrdCenter, nrrd->axis[ai].center),
              nrrd->axis[ai].size);
      airSinglePrintf(file, NULL, "spacing=%lg, \n", nrrd->axis[ai].spacing);
      airSinglePrintf(file, NULL, "thickness=%lg, \n",
                      nrrd->axis[ai].thickness);
      airSinglePrintf(file, NULL, "    axis(Min,Max) = (%lg,",
                       nrrd->axis[ai].min);
      airSinglePrintf(file, NULL, "%lg)\n", nrrd->axis[ai].max);
      if (airStrlen(nrrd->axis[ai].units)) {
        fprintf(file, "units=%s, \n", nrrd->axis[ai].units);
      }
    }
    /*
    airSinglePrintf(file, NULL, "The min, max values are %lg",
                     nrrd->min);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->max);
    */
    airSinglePrintf(file, NULL, "The old min, old max values are %lg",
                     nrrd->oldMin);
    airSinglePrintf(file, NULL, ", %lg\n", nrrd->oldMax);
    /* fprintf(file, "hasNonExist = %d\n", nrrd->hasNonExist); */
    if (nrrd->cmtArr->len) {
      fprintf(file, "Comments:\n");
      for (ai=0; ai<nrrd->cmtArr->len; ai++) {
        fprintf(file, "%s\n", nrrd->cmt[ai]);
      }
    }
    fprintf(file, "\n");
  }
}

/*
** asserts all the properties associated with orientation information
**
** The most important part of this is asserting the per-axis mutual 
** exclusion of min/max/spacing/units versus using spaceDirection.
*/
int
_nrrdFieldCheckSpaceInfo(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheckSpaceInfo", err[BIFF_STRLEN];
  unsigned int dd, ii;
  int exists;

  if (!( !nrrd->space || !airEnumValCheck(nrrdSpace, nrrd->space) )) {
    sprintf(err, "%s: space %d invalid", me, nrrd->space);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  if (!( nrrd->spaceDim <= NRRD_SPACE_DIM_MAX )) {
    sprintf(err, "%s: space dimension %d is outside valid range "
            "[0,NRRD_SPACE_DIM_MAX] = [0,%d]",
            me, nrrd->dim, NRRD_SPACE_DIM_MAX);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  if (nrrd->spaceDim) {
    if (nrrd->space) {
      if (nrrdSpaceDimension(nrrd->space) != nrrd->spaceDim) {
        sprintf(err, "%s: space %s has dimension %d but spaceDim is %d", 
                me, airEnumStr(nrrdSpace, nrrd->space),
                nrrdSpaceDimension(nrrd->space), nrrd->spaceDim);
        biffMaybeAdd(NRRD, err, useBiff); return 1;
      }
    }
    /* check that all coeffs of spaceOrigin have consistent existance */
    exists = AIR_EXISTS(nrrd->spaceOrigin[0]);
    for (ii=0; ii<nrrd->spaceDim; ii++) {
      if (exists ^ AIR_EXISTS(nrrd->spaceOrigin[ii])) {
        sprintf(err, "%s: existance of space origin coefficients must "
                "be consistent (val[0] not like val[%d])", me, ii);
        biffMaybeAdd(NRRD, err, useBiff); return 1;
      }
    }
    /* check that all coeffs of measurementFrame have consistent existance */
    exists = AIR_EXISTS(nrrd->measurementFrame[0][0]);
    for (dd=0; dd<nrrd->spaceDim; dd++) {
      for (ii=0; ii<nrrd->spaceDim; ii++) {
        if (exists ^ AIR_EXISTS(nrrd->measurementFrame[dd][ii])) {
          sprintf(err, "%s: existance of measurement frame coefficients must "
                  "be consistent: [col][row] [%d][%d] not like [0][0])",
                  me, dd, ii);
          biffMaybeAdd(NRRD, err, useBiff); return 1;
        }
      }
    }
    /* check on space directions */
    for (dd=0; dd<nrrd->dim; dd++) {
      exists = AIR_EXISTS(nrrd->axis[dd].spaceDirection[0]);
      for (ii=1; ii<nrrd->spaceDim; ii++) {
        if (exists ^ AIR_EXISTS(nrrd->axis[dd].spaceDirection[ii])) {
          sprintf(err, "%s: existance of space direction %d coefficients "
                  "must be consistent (val[0] not like val[%d])", me,
                  dd, ii);
          biffMaybeAdd(NRRD, err, useBiff); return 1;
        }
      }
      if (exists) {
        if (AIR_EXISTS(nrrd->axis[dd].min)
            || AIR_EXISTS(nrrd->axis[dd].max)
            || AIR_EXISTS(nrrd->axis[dd].spacing)
            || airStrlen(nrrd->axis[dd].units)) {
          sprintf(err, "%s: axis[%d] has a direction vector, and so can't "
                  "have min, max, spacing, or units set", me, dd);
          biffMaybeAdd(NRRD, err, useBiff); return 1;
        }
      }
    }
  } else {
    /* else there's not supposed to be anything in "space" */
    if (nrrd->space) {
      sprintf(err, "%s: space %s can't be set with spaceDim %d", 
              me, airEnumStr(nrrdSpace, nrrd->space), nrrd->spaceDim);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    /* -------- */
    exists = AIR_FALSE;
    for (dd=0; dd<NRRD_SPACE_DIM_MAX; dd++) {
      exists |= airStrlen(nrrd->spaceUnits[dd]);
    }
    if (exists) {
      sprintf(err, "%s: spaceDim is 0, but space units is set", me);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    /* -------- */
    exists = AIR_FALSE;
    for (dd=0; dd<NRRD_SPACE_DIM_MAX; dd++) {
      exists |= AIR_EXISTS(nrrd->spaceOrigin[dd]);
    }
    if (exists) {
      sprintf(err, "%s: spaceDim is 0, but space origin is set", me);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    /* -------- */
    exists = AIR_FALSE;
    for (dd=0; dd<NRRD_SPACE_DIM_MAX; dd++) {
      for (ii=0; ii<NRRD_DIM_MAX; ii++) {
        exists |= AIR_EXISTS(nrrd->axis[ii].spaceDirection[dd]);
      }
    }
    if (exists) {
      sprintf(err, "%s: spaceDim is 0, but space directions are set", me);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  return 0;
}

/* --------------------- per-field checks ----------------
**
** Strictly speacking, these checks only apply to the nrrd itself, not
** to a potentially incomplete nrrd in the process of being read, so
** the NrrdIoState stuff is not an issue.  This limits the utility of
** these to the field parsers for handling the more complex state 
** involved in parsing some of the NRRD fields (like units).
**
** return 0 if it is valid, and 1 if there is an error
*/

int
_nrrdFieldCheck_noop(const Nrrd *nrrd, int useBiff) {

  AIR_UNUSED(nrrd);
  AIR_UNUSED(useBiff);
  return 0;
}

int
_nrrdFieldCheck_type(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_type", err[BIFF_STRLEN];
  
  if (airEnumValCheck(nrrdType, nrrd->type)) {
    sprintf(err, "%s: type (%d) is not valid", me, nrrd->type);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_block_size(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_block_size", err[BIFF_STRLEN];
  
  if (nrrdTypeBlock == nrrd->type && (!(0 < nrrd->blockSize)) ) {
    sprintf(err, "%s: type is %s but nrrd->blockSize (" 
            _AIR_SIZE_T_CNV ") invalid", me,
            airEnumStr(nrrdType, nrrdTypeBlock),
            nrrd->blockSize);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  if (nrrdTypeBlock != nrrd->type && (0 < nrrd->blockSize)) {
    sprintf(err, "%s: type is %s (not block) but blockSize is "
            _AIR_SIZE_T_CNV, me,
            airEnumStr(nrrdType, nrrd->type), nrrd->blockSize);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_dimension(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_dimension", err[BIFF_STRLEN];
  
  if (!AIR_IN_CL(1, nrrd->dim, NRRD_DIM_MAX)) {
    sprintf(err, "%s: dimension %u is outside valid range [1,%d]",
            me, nrrd->dim, NRRD_DIM_MAX);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_space(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_space", err[BIFF_STRLEN];

  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_space_dimension(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_space_dimension", err[BIFF_STRLEN];
  
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_sizes(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_sizes", err[BIFF_STRLEN];
  size_t size[NRRD_DIM_MAX];
  
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSize, size);
  if (_nrrdSizeCheck(size, nrrd->dim, useBiff)) {
    sprintf(err, "%s: trouble with array sizes", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_spacings(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_spacings", err[BIFF_STRLEN];
  double val[NRRD_DIM_MAX];
  unsigned int ai;

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSpacing, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    if (!( !airIsInf_d(val[ai]) && (airIsNaN(val[ai]) || (0 != val[ai])) )) {
      sprintf(err, "%s: axis %d spacing (%g) invalid", me, ai, val[ai]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_thicknesses(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_thicknesses", err[BIFF_STRLEN];
  double val[NRRD_DIM_MAX];
  unsigned int ai;

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoThickness, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    /* note that unlike spacing, we allow zero thickness, 
       but it makes no sense to be negative */
    if (!( !airIsInf_d(val[ai]) && (airIsNaN(val[ai]) || (0 <= val[ai])) )) {
      sprintf(err, "%s: axis %d thickness (%g) invalid", me, ai, val[ai]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  return 0;
}

int
_nrrdFieldCheck_axis_mins(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_axis_mins", err[BIFF_STRLEN];
  double val[NRRD_DIM_MAX];
  unsigned int ai;
  int ret;

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoMin, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    if ((ret=airIsInf_d(val[ai]))) {
      sprintf(err, "%s: axis %d min %sinf invalid",
              me, ai, 1==ret ? "+" : "-");
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  /* HEY: contemplate checking min != max, but what about stub axes ... */
  return 0;
}

int
_nrrdFieldCheck_axis_maxs(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_axis_maxs", err[BIFF_STRLEN];
  double val[NRRD_DIM_MAX];
  unsigned int ai;
  int ret;

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoMax, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    if ((ret=airIsInf_d(val[ai]))) {
      sprintf(err, "%s: axis %d max %sinf invalid",
              me, ai, 1==ret ? "+" : "-");
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: trouble", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  /* HEY: contemplate checking min != max, but what about stub axes ... */
  return 0;
}

int
_nrrdFieldCheck_space_directions(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_space_directions", err[BIFF_STRLEN];

  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: space info problem", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_centers(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_centers", err[BIFF_STRLEN];
  unsigned int ai;
  int val[NRRD_DIM_MAX];

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoCenter, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    if (!( nrrdCenterUnknown == val[ai]
           || !airEnumValCheck(nrrdCenter, val[ai]) )) {
      sprintf(err, "%s: axis %d center %d invalid", me, ai, val[ai]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  return 0;
}

int
_nrrdFieldCheck_kinds(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_kinds", err[BIFF_STRLEN];
  int val[NRRD_DIM_MAX];
  unsigned int wantLen, ai;

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoKind, val);
  for (ai=0; ai<nrrd->dim; ai++) {
    if (!( nrrdKindUnknown == val[ai]
           || !airEnumValCheck(nrrdKind, val[ai]) )) {
      sprintf(err, "%s: axis %d kind %d invalid", me, ai, val[ai]);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
    wantLen = nrrdKindSize(val[ai]);
    if (wantLen && wantLen != nrrd->axis[ai].size) {
      sprintf(err, "%s: axis %d kind %s requires size %d, but have "
              _AIR_SIZE_T_CNV, me,
              ai, airEnumStr(nrrdKind, val[ai]), wantLen, nrrd->axis[ai].size);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  return 0;
}

int
_nrrdFieldCheck_labels(const Nrrd *nrrd, int useBiff) {
  /* char me[]="_nrrdFieldCheck_labels", err[BIFF_STRLEN]; */

  AIR_UNUSED(nrrd);
  AIR_UNUSED(useBiff);

  /* don't think there's anything to do here: the label strings are
     either NULL (which is okay) or non-NULL, but we have no restrictions
     on the validity of the strings */

  return 0;
}

int
_nrrdFieldCheck_units(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_units", err[BIFF_STRLEN];

  /* as with labels- the strings themselves don't need checking themselves */
  /* but per-axis units cannot be set for axes with space directions ... */
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: space info problem", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_old_min(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_old_min", err[BIFF_STRLEN];
  int ret;

  if ((ret=airIsInf_d(nrrd->oldMin))) {
    sprintf(err, "%s: old min %sinf invalid", me, 1==ret ? "+" : "-");
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  /* oldMin == oldMax is perfectly valid */
  return 0;
}

int
_nrrdFieldCheck_old_max(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_old_max", err[BIFF_STRLEN];
  int ret;

  if ((ret=airIsInf_d(nrrd->oldMax))) {
    sprintf(err, "%s: old max %sinf invalid", me, 1==ret ? "+" : "-");
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  /* oldMin == oldMax is perfectly valid */
  return 0;
}

int
_nrrdFieldCheck_keyvalue(const Nrrd *nrrd, int useBiff) {
  /* char me[]="_nrrdFieldCheck_keyvalue", err[BIFF_STRLEN]; */

  AIR_UNUSED(nrrd);
  AIR_UNUSED(useBiff);

  /* nrrdKeyValueAdd() ensures that keys aren't repeated, 
     not sure what other kind of checking can be done */

  return 0;
}

int
_nrrdFieldCheck_space_units(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_space_units", err[BIFF_STRLEN];

  /* not sure if there's anything to specifically check for the
     space units themselves ... */
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: space info problem", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_space_origin(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_space_origin", err[BIFF_STRLEN];

  /* pre-Fri Feb 11 04:25:36 EST 2005, I thought that 
     the spaceOrigin must be known to describe the 
     space/orientation stuff, but that's too restrictive,
     which is why below says AIR_FALSE instead of AIR_TRUE */
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: space info problem", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
_nrrdFieldCheck_measurement_frame(const Nrrd *nrrd, int useBiff) {
  char me[]="_nrrdFieldCheck_measurement_frame", err[BIFF_STRLEN];
  
  if (_nrrdFieldCheckSpaceInfo(nrrd, useBiff)) {
    sprintf(err, "%s: space info problem", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  return 0;
}

int
(*_nrrdFieldCheck[NRRD_FIELD_MAX+1])(const Nrrd *, int useBiff) = {
  _nrrdFieldCheck_noop,           /* nonfield */
  _nrrdFieldCheck_noop,           /* comment */
  _nrrdFieldCheck_noop,           /* content */
  _nrrdFieldCheck_noop,           /* number */
  _nrrdFieldCheck_type,
  _nrrdFieldCheck_block_size,
  _nrrdFieldCheck_dimension,
  _nrrdFieldCheck_space,
  _nrrdFieldCheck_space_dimension,
  _nrrdFieldCheck_sizes,
  _nrrdFieldCheck_spacings,
  _nrrdFieldCheck_thicknesses,
  _nrrdFieldCheck_axis_mins,
  _nrrdFieldCheck_axis_maxs,
  _nrrdFieldCheck_space_directions,
  _nrrdFieldCheck_centers,
  _nrrdFieldCheck_kinds,
  _nrrdFieldCheck_labels,
  _nrrdFieldCheck_units,
  _nrrdFieldCheck_noop,           /* min */
  _nrrdFieldCheck_noop,           /* max */
  _nrrdFieldCheck_old_min,
  _nrrdFieldCheck_old_max,
  _nrrdFieldCheck_noop,           /* endian */
  _nrrdFieldCheck_noop,           /* encoding */
  _nrrdFieldCheck_noop,           /* line_skip */
  _nrrdFieldCheck_noop,           /* byte_skip */
  _nrrdFieldCheck_keyvalue,
  _nrrdFieldCheck_noop,           /* sample units */
  _nrrdFieldCheck_space_units,
  _nrrdFieldCheck_space_origin,
  _nrrdFieldCheck_measurement_frame,
  _nrrdFieldCheck_noop,           /* data_file */
};

int
_nrrdCheck(const Nrrd *nrrd, int checkData, int useBiff) {
  char me[]="_nrrdCheck", err[BIFF_STRLEN];
  int fi;

  if (!nrrd) {
    sprintf(err, "%s: got NULL pointer", me);
    biffMaybeAdd(NRRD, err, useBiff); return 1;
  }
  if (checkData) {
    if (!(nrrd->data)) {
      sprintf(err, "%s: nrrd has NULL data pointer", me);
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  for (fi=nrrdField_unknown+1; fi<nrrdField_last; fi++) {
    /* yes, this will call _nrrdFieldCheckSpaceInfo() many many times */
    if (_nrrdFieldCheck[fi](nrrd, AIR_TRUE)) {
      sprintf(err, "%s: trouble with %s field", me,
              airEnumStr(nrrdField, fi));
      biffMaybeAdd(NRRD, err, useBiff); return 1;
    }
  }
  return 0;
}

/*
******** nrrdCheck()
**
** does some consistency checks for things that can go wrong in a nrrd
** returns non-zero if there is a problem, zero if no problem.
**
** You might think that this should be merged with _nrrdHeaderCheck(),
** but that is really only for testing sufficiency of information 
** required to do the data reading.
*/
int
nrrdCheck(const Nrrd *nrrd) {
  char me[]="nrrdCheck", err[BIFF_STRLEN];

  if (_nrrdCheck(nrrd, AIR_TRUE, AIR_TRUE)) {
    sprintf(err, "%s: trouble", me);
    biffAdd(NRRD, err); return 1;
  }
  return 0;
}

/*
******** nrrdSameSize()
**
** returns 1 iff given two nrrds have same dimension and axes sizes.
** This does NOT look at the type of the elements.
**
** The intended user of this is someone who really wants the nrrds to be
** the same size, so that if they aren't, some descriptive (error) message
** can be generated according to useBiff
*/
int
nrrdSameSize (const Nrrd *n1, const Nrrd *n2, int useBiff) {
  char me[]="nrrdSameSize", err[BIFF_STRLEN];
  unsigned int ai;

  if (!(n1 && n2)) {
    sprintf(err, "%s: got NULL pointer", me);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  if (n1->dim != n2->dim) {
    sprintf(err, "%s: n1->dim (%d) != n2->dim (%d)", me, n1->dim, n2->dim);
    biffMaybeAdd(NRRD, err, useBiff); 
    return 0;
  }
  for (ai=0; ai<n1->dim; ai++) {
    if (n1->axis[ai].size != n2->axis[ai].size) {
      sprintf(err, "%s: n1->axis[%d].size (" _AIR_SIZE_T_CNV
              ") != n2->axis[%d].size (" _AIR_SIZE_T_CNV ")", 
              me, ai, n1->axis[ai].size, ai, n2->axis[ai].size);
      biffMaybeAdd(NRRD, err, useBiff); 
      return 0;
    }
  }
  return 1;
}

/*
******** nrrdElementSize()
**
** So just how many bytes long is one element in this nrrd?  This is
** needed (over the simple nrrdTypeSize[] array) because some nrrds
** may be of "block" type, and because it does bounds checking on
** nrrd->type.  Returns 0 if given a bogus nrrd->type, or if the block
** size isn't greater than zero (in which case it sets nrrd->blockSize
** to 0, just out of spite).  This function never returns a negative
** value; using (!nrrdElementSize(nrrd)) is a sufficient check for
** invalidity.
**
** Besides learning how many bytes long one element is, this function
** is useful as a way of detecting an invalid blocksize on a block nrrd.
*/
size_t
nrrdElementSize (const Nrrd *nrrd) {

  if (!( nrrd && !airEnumValCheck(nrrdType, nrrd->type) )) {
    return 0;
  }
  if (nrrdTypeBlock != nrrd->type) {
    return nrrdTypeSize[nrrd->type];
  }
  /* else its block type */
  if (nrrd->blockSize > 0) {
    return nrrd->blockSize;
  }
  /* else we got an invalid block size */
  /* nrrd->blockSize = 0; */
  return 0;
}

/*
******** nrrdElementNumber()
**
** takes the place of old "nrrd->num": the number of elements in the
** nrrd, which is just the product of the axis sizes.  A return of 0
** means there's a problem.  Negative numbers are never returned.
**
** does NOT use biff
*/
size_t
nrrdElementNumber (const Nrrd *nrrd) {
  size_t num, size[NRRD_DIM_MAX];
  unsigned int ai; 

  if (!nrrd) {
    return 0;
  }
  /* else */
  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSize, size);
  if (_nrrdSizeCheck(size, nrrd->dim, AIR_FALSE)) {
    /* the nrrd's size information is invalid, can't proceed */
    return 0;
  }
  num = 1;
  for (ai=0; ai<nrrd->dim; ai++) {
    /* negative numbers and overflow were caught by _nrrdSizeCheck() */
    num *= size[ai];
  }
  return num;
}

/*
** obviously, this requires that the per-axis size fields have been set
*/
void
_nrrdSplitSizes(size_t *pieceSize, size_t *pieceNum, Nrrd *nrrd,
                unsigned int split) {
  unsigned int ai;
  size_t size[NRRD_DIM_MAX];

  nrrdAxisInfoGet_nva(nrrd, nrrdAxisInfoSize, size);
  *pieceSize = 1;
  for (ai=0; ai<split; ai++) {
    *pieceSize *= size[ai];
  }
  *pieceNum = 1;
  for (ai=split; ai<nrrd->dim; ai++) {
    *pieceNum *= size[ai];
  }
  return;
}

/*
******** nrrdHasNonExistSet()
**
** This function will always (assuming type is valid) set the value of
** nrrd->hasNonExist to either nrrdNonExistTrue or nrrdNonExistFalse,
** and it will return that value.  For lack of a more sophisticated
** policy, blocks are currently always considered to be existant
** values (because nrrdTypeIsIntegral[nrrdTypeBlock] is currently true).
** This function will ALWAYS determine the correct answer and set the
** value of nrrd->hasNonExist: it ignores the value of
** nrrd->hasNonExist on the input nrrd.  Exception: if nrrd is null or
** type is bogus, no action is taken and nrrdNonExistUnknown is
** returned.
**
** Because this will return either nrrdNonExistTrue or nrrdNonExistFalse,
** and because the C boolean value of these are true and false (respectively),
** it is possible (and encouraged) to use the return of this function
** as the expression of a conditional:
**
**   if (nrrdHasNonExistSet(nrrd)) {
**     ... handle existance of non-existant values ...
**   }
*/
/*
int
nrrdHasNonExistSet(Nrrd *nrrd) {
  size_t I, N;
  float val;

  if (!( nrrd && !airEnumValCheck(nrrdType, nrrd->type) ))
    return nrrdNonExistUnknown;

  if (nrrdTypeIsIntegral[nrrd->type]) {
    nrrd->hasNonExist = nrrdNonExistFalse;
  } else {
    nrrd->hasNonExist = nrrdNonExistFalse;
    N = nrrdElementNumber(nrrd);
    for (I=0; I<N; I++) {
      val = nrrdFLookup[nrrd->type](nrrd->data, I);
      if (!AIR_EXISTS(val)) {
        nrrd->hasNonExist = nrrdNonExistTrue;
        break;
      }
    }
  }
  return nrrd->hasNonExist;
}
*/

int
_nrrdCheckEnums(void) {
  char me[]="_nrrdCheckEnums", err[BIFF_STRLEN],
    which[AIR_STRLEN_SMALL];

  if (nrrdFormatTypeLast-1 != NRRD_FORMAT_TYPE_MAX) {
    strcpy(which, "nrrdFormat"); goto err;
  }
  if (nrrdTypeLast-1 != NRRD_TYPE_MAX) {
    strcpy(which, "nrrdType"); goto err;
  }
  if (nrrdEncodingTypeLast-1 != NRRD_ENCODING_TYPE_MAX) {
    strcpy(which, "nrrdEncodingType"); goto err;
  }
  if (nrrdCenterLast-1 != NRRD_CENTER_MAX) {
    strcpy(which, "nrrdCenter"); goto err;
  }
  if (nrrdAxisInfoLast-1 != NRRD_AXIS_INFO_MAX) {
    strcpy(which, "nrrdAxisInfo"); goto err;
  }
  /* can't really check on endian enum */
  if (nrrdField_last-1 != NRRD_FIELD_MAX) {
    strcpy(which, "nrrdField"); goto err;
  }
  if (nrrdHasNonExistLast-1 != NRRD_HAS_NON_EXIST_MAX) {
    strcpy(which, "nrrdHasNonExist"); goto err;
  }
  /* ---- BEGIN non-NrrdIO */
  if (nrrdBoundaryLast-1 != NRRD_BOUNDARY_MAX) {
    strcpy(which, "nrrdBoundary"); goto err;
  }
  if (nrrdMeasureLast-1 != NRRD_MEASURE_MAX) {
    strcpy(which, "nrrdMeasure"); goto err;
  }
  if (nrrdUnaryOpLast-1 != NRRD_UNARY_OP_MAX) {
    strcpy(which, "nrrdUnaryOp"); goto err;
  }
  if (nrrdBinaryOpLast-1 != NRRD_BINARY_OP_MAX) {
    strcpy(which, "nrrdBinaryOp"); goto err;
  }
  if (nrrdTernaryOpLast-1 != NRRD_TERNARY_OP_MAX) {
    strcpy(which, "nrrdTernaryOp"); goto err;
  }
  /* ---- END non-NrrdIO */
  
  /* no errors so far */
  return 0;

 err:
  sprintf(err, "%s: Last vs. MAX incompatibility for %s enum", me, which);
  biffAdd(NRRD, err); return 1;
}

/*
******** nrrdSanity()
**
** makes sure that all the basic assumptions of nrrd hold for
** the architecture/etc which we're currently running on.  
** 
** returns 1 if all is okay, 0 if there is a problem
*/
int
nrrdSanity(void) {
  char me[]="nrrdSanity", err[BIFF_STRLEN];
  int aret, type;
  size_t maxsize;
  airLLong tmpLLI;
  airULLong tmpULLI;
  static int _nrrdSanity = 0;

  if (_nrrdSanity) {
    /* we've been through this once before and things looked okay ... */
    /* Is this thread-safe?  I think so.  If we assume that any two
       threads are going to compute the same value, isn't it the case
       that, at worse, both of them will go through all the tests and
       then set _nrrdSanity to the same thing? */
    return 1;
  }
  
  aret = airSanity();
  if (aret != airInsane_not) {
    sprintf(err, "%s: airSanity() failed: %s", me, airInsaneErr(aret));
    biffAdd(NRRD, err); return 0;
  }

  if (airEnumValCheck(nrrdEncodingType, nrrdDefaultWriteEncodingType)) {
    sprintf(err, "%s: nrrdDefaultWriteEncodingType (%d) not in valid "
            "range [%d,%d]", me, nrrdDefaultWriteEncodingType,
            nrrdEncodingTypeUnknown+1, nrrdEncodingTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdCenter, nrrdDefaultCenter)) {
    sprintf(err, "%s: nrrdDefaultCenter (%d) not in valid range [%d,%d]",
            me, nrrdDefaultCenter,
            nrrdCenterUnknown+1, nrrdCenterLast-1);
    biffAdd(NRRD, err); return 0;
  }
  /* ---- BEGIN non-NrrdIO */
  if (!( nrrdTypeDefault == nrrdDefaultResampleType
         || !airEnumValCheck(nrrdType, nrrdDefaultResampleType) )) {
    sprintf(err, "%s: nrrdDefaultResampleType (%d) not in valid range [%d,%d]",
            me, nrrdDefaultResampleType,
            nrrdTypeUnknown, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdBoundary, nrrdDefaultResampleBoundary)) {
    sprintf(err, "%s: nrrdDefaultResampleBoundary (%d) "
            "not in valid range [%d,%d]",
            me, nrrdDefaultResampleBoundary,
            nrrdBoundaryUnknown+1, nrrdBoundaryLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdType, nrrdStateMeasureType)) {
    sprintf(err, "%s: nrrdStateMeasureType (%d) not in valid range [%d,%d]",
            me, nrrdStateMeasureType,
            nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  if (airEnumValCheck(nrrdType, nrrdStateMeasureHistoType)) {
    sprintf(err,
            "%s: nrrdStateMeasureHistoType (%d) not in valid range [%d,%d]",
            me, nrrdStateMeasureType,
            nrrdTypeUnknown+1, nrrdTypeLast-1);
    biffAdd(NRRD, err); return 0;
  }
  /* ---- END non-NrrdIO */

  if (!( nrrdTypeSize[nrrdTypeChar] == sizeof(char)
         && nrrdTypeSize[nrrdTypeUChar] == sizeof(unsigned char)
         && nrrdTypeSize[nrrdTypeShort] == sizeof(short)
         && nrrdTypeSize[nrrdTypeUShort] == sizeof(unsigned short)
         && nrrdTypeSize[nrrdTypeInt] == sizeof(int)
         && nrrdTypeSize[nrrdTypeUInt] == sizeof(unsigned int)
         && nrrdTypeSize[nrrdTypeLLong] == sizeof(airLLong)
         && nrrdTypeSize[nrrdTypeULLong] == sizeof(airULLong)
         && nrrdTypeSize[nrrdTypeFloat] == sizeof(float)
         && nrrdTypeSize[nrrdTypeDouble] == sizeof(double) )) {
    sprintf(err, "%s: sizeof() for nrrd types has problem: "
            "expected (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d) "
            "but got (%d,%d,%d,%d,%d,%d,%d,%d,%d,%d)", me,
            (int)nrrdTypeSize[nrrdTypeChar],
            (int)nrrdTypeSize[nrrdTypeUChar],
            (int)nrrdTypeSize[nrrdTypeShort],
            (int)nrrdTypeSize[nrrdTypeUShort],
            (int)nrrdTypeSize[nrrdTypeInt],
            (int)nrrdTypeSize[nrrdTypeUInt],
            (int)nrrdTypeSize[nrrdTypeLLong],
            (int)nrrdTypeSize[nrrdTypeULLong],
            (int)nrrdTypeSize[nrrdTypeFloat],
            (int)nrrdTypeSize[nrrdTypeDouble],
            (int)sizeof(char),
            (int)sizeof(unsigned char),
            (int)sizeof(short),
            (int)sizeof(unsigned short),
            (int)sizeof(int),
            (int)sizeof(unsigned int),
            (int)sizeof(airLLong),
            (int)sizeof(airULLong),
            (int)sizeof(float),
            (int)sizeof(double));
    biffAdd(NRRD, err); return 0;
  }

  /* check on NRRD_TYPE_SIZE_MAX */
  maxsize = 0;
  for (type=nrrdTypeUnknown+1; type<=nrrdTypeLast-2; type++) {
    maxsize = AIR_MAX(maxsize, nrrdTypeSize[type]);
  }
  if (maxsize != NRRD_TYPE_SIZE_MAX) {
    sprintf(err, "%s: actual max type size is %d != %d == NRRD_TYPE_SIZE_MAX",
            me, (int)maxsize, NRRD_TYPE_SIZE_MAX);
    biffAdd(NRRD, err); return 0;
  }

  /* check on NRRD_TYPE_BIGGEST */
  if (maxsize != sizeof(NRRD_TYPE_BIGGEST)) {
    sprintf(err, "%s: actual max type size is %d != "
            "%d == sizeof(NRRD_TYPE_BIGGEST)",
            me, (int)maxsize, (int)sizeof(NRRD_TYPE_BIGGEST));
    biffAdd(NRRD, err); return 0;
  }
  
  /* nrrd-defined type min/max values */
  tmpLLI = NRRD_LLONG_MAX;
  if (tmpLLI != NRRD_LLONG_MAX) {
    sprintf(err, "%s: long long int can't hold NRRD_LLONG_MAX ("
            AIR_ULLONG_FMT ")", me,
            NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpLLI += 1;
  if (NRRD_LLONG_MIN != tmpLLI) {
    sprintf(err, "%s: long long int min (" AIR_LLONG_FMT ") or max ("
            AIR_LLONG_FMT ") incorrect", me,
            NRRD_LLONG_MIN, NRRD_LLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI = NRRD_ULLONG_MAX;
  if (tmpULLI != NRRD_ULLONG_MAX) {
    sprintf(err, 
            "%s: unsigned long long int can't hold NRRD_ULLONG_MAX ("
            AIR_ULLONG_FMT ")",
            me, NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }
  tmpULLI += 1;
  if (tmpULLI != 0) {
    sprintf(err, "%s: unsigned long long int max (" AIR_ULLONG_FMT 
            ") incorrect", me,
            NRRD_ULLONG_MAX);
    biffAdd(NRRD, err); return 0;
  }

  if (_nrrdCheckEnums()) {
    sprintf(err, "%s: problem with enum definition", me);
    biffAdd(NRRD, err); return 0;
  }
  
  if (!( NRRD_DIM_MAX >= 3 )) {
    sprintf(err, "%s: NRRD_DIM_MAX == %d seems awfully small, doesn't it?",
            me, NRRD_DIM_MAX);
    biffAdd(NRRD, err); return 0;
  }

  if (!nrrdTypeIsIntegral[nrrdTypeBlock]) {
    sprintf(err, "%s: nrrdTypeInteger[nrrdTypeBlock] is not true, things "
            "could get wacky", me);
    biffAdd(NRRD, err); return 0;
  }

  /* HEY: any other assumptions built into Teem? */

  _nrrdSanity = 1;
  return 1;
}
