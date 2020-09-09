# KFold Cross Validation

## Overview

$k$-fold cross validation is a [cross validation](CrossValidation/index.md) method that splits data into $k$ train-tests sets. The parameter [!param](/CrossValidation/KFold/num_folds) specifies the number of sets created which splits the data evenly so that every data point is in a testing set once and in the training set $k-1$ times. [Wikipedia](https://en.wikipedia.org/wiki/Cross-validation_%28statistics%29#k-fold_cross-validation) has a good description of $k$-fold cross validation.

The [!param](/CrossValidation/KFold/shuffle) parameter allows the user to specify whether the data should be randomly shuffled before the train-test sets are defined. This is typically a good idea if the data is ordered in some way. By default, the system will automatically determine whether the data should be shuffled or not based on the type of sampler the trainer uses. [CartesianProductSampler.md] and [QuadratureSampler.md] samplers will set [!param](/CrossValidation/KFold/shuffle) `= true`.

## Example Input File Syntax

Below shows an example of using $k$-fold cross validation on a [PolynomialRegressionSurrogate.md].

!listing cv_kfold.i block=CrossValidation

!syntax parameters /CrossValidation/KFold

!syntax list /CrossValidation/KFold objects=True actions=False subsystems=False

!syntax list /CrossValidation/KFold objects=False actions=False subsystems=True

!syntax list /CrossValidation/KFold objects=False actions=True subsystems=False
