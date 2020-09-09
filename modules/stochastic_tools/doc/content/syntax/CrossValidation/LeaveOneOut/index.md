# Leave-One-Out Cross Validation

## Overview

Leave-one-out (LOO) cross validation is a [cross validation](CrossValidation/index.md) method that splits the data such that a surrogate is trained with one data point is left out, then that data point is used in the test set. In other words, there will be $N$ train-test sets with $N-1$ training point and 1 testing point, where $N$ is the number of point in the data. See [wikipedia](https://en.wikipedia.org/wiki/Cross-validation_%28statistics%29#Leave-one-out_cross-validation) for more details.

If the data set is large, then LOO might become too burdensome, so typically [$k$-fold](KFold/index.md) is recommended. However, linear models such as [PolynomialRegressionSurrogate.md] and [PolynomialChaos.md] are able to compute the LOO root-mean-squared-error (RMSE) without retraining for each train-test split. When the system detects that the surrogate is a linear model, the $N$ trainers will not be constructed and instead computes the LOO-RMSE using [LinearLOOCrossValidation](LinearLOOCrossValidation.md), unless [!param](/CrossValidation/LeaveOneOut/force_retrain) is set to `true`.

## Example Input File Syntax

Below is an example of performing LOO cross validation on [polynomial regression](PolynomialRegressionSurrogate.md) and [polynomial chaos](PolynomialChaos.md) surrogates.

!listing cv_loo.i block=CrossValidation

!syntax parameters /CrossValidation/LeaveOneOut

!syntax list /CrossValidation/LeaveOneOut objects=True actions=False subsystems=False

!syntax list /CrossValidation/LeaveOneOut objects=False actions=False subsystems=True

!syntax list /CrossValidation/LeaveOneOut objects=False actions=True subsystems=False
