# LinearLOOCrossValidation

!syntax description /Postprocessors/LinearLOOCrossValidation

## Overview

This object computes the leave-one-out cross validated root mean squared error for linear surrogate models. Currently, this has been only implemented for [PolynomialRegressionSurrogate.md] and [PolynomialChaos.md]. See [Leave-one-out cross-validation](https://en.wikipedia.org/wiki/Cross-validation_(statistics)#Leave-one-out_cross-validation) for more information on the methodology and [Fast computation of cross-validation in linear models](https://robjhyndman.com/hyndsight/loocv-linear-models/) for detail on the implementation.

!syntax parameters /Postprocessors/LinearLOOCrossValidation

!syntax inputs /Postprocessors/LinearLOOCrossValidation

!syntax children /Postprocessors/LinearLOOCrossValidation
