/* This file is part of the PROBA2 Science Operations Center software.
 * Copyright (C) 2007-2014 Royal Observatory of Belgium.
 * For copying permission, see the file COPYING in the distribution.
 *
 * Author: Bogdan Nicula
 */

#include <lcms2.h>

int main(void) {
    cmsCIExyY d65;
    cmsCIEXYZ D65;

    cmsToneCurve *gamma = cmsBuildGamma(NULL, 2.2);
    cmsHPROFILE hProfile = cmsOpenProfileFromFile("sidc_gray.icc", "w");

    cmsWhitePointFromTemp(&d65, 6504);
    cmsxyY2XYZ(&D65, &d65);

    cmsSetDeviceClass(hProfile, cmsSigDisplayClass);
    cmsSetColorSpace(hProfile, cmsSigGrayData);
    cmsSetPCS(hProfile, cmsSigXYZData);
    cmsSetHeaderRenderingIntent(hProfile, INTENT_PERCEPTUAL);

    cmsWriteTag(hProfile, cmsSigGrayTRCTag, gamma);
    cmsWriteTag(hProfile, cmsSigMediaWhitePointTag, &D65);

    cmsMLU *mlu = NULL;

    mlu = cmsMLUalloc(NULL, 1);
    cmsMLUsetASCII(mlu, cmsNoLanguage, cmsNoCountry, "SIDC Gray Gamma 2.2");
    cmsWriteTag(hProfile, cmsSigProfileDescriptionTag, mlu);
    cmsMLUfree(mlu);

    mlu = cmsMLUalloc(NULL, 1);
    cmsMLUsetASCII(mlu, cmsNoLanguage, cmsNoCountry, "Public Domain");
    cmsWriteTag(hProfile, cmsSigCopyrightTag, mlu);
    cmsMLUfree(mlu);

    cmsCloseProfile(hProfile);
    cmsFreeToneCurve(gamma);

    return 0;
}
