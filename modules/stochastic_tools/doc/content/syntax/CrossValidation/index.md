# Cross Validation System

## Overview

Cross validation is a method to quantify the goodness of fit for [surrogate models](Trainers/index.md). The idea is to split the data into sets of training and test data. So a different surrogate is trained on each set of training data and tested on each set of testing data. Cross validation is useful to see how sensitive the surrogate model is to outliers and biases in the data. [Wikipedia](https://en.wikipedia.org/wiki/Cross-validation_%28statistics%29) offers a great resource for understanding cross validation.

All cross validation systems produce a [trainer](Trainers/index.md) and [surrogate](Surrogates/index.md) object for each train-test set, and a [CrossValidatedRSME.md] postprocessor is created to compute the cross validated root-mean-square-error.

## Example Input File Syntax

All cross validation implementations in MOOSE require three input parameters:

1. [!param](/CrossValidation/KFold/trainer): training object to perform cross validation on.
1. [!param](/CrossValidation/KFold/surrogate): surrogate object to copy for each train-test set.
1. [!param](/CrossValidation/KFold/num_rows): number of rows in sampler, i.e. data set size.

Unfortunately, the [!param](/CrossValidation/KFold/num_rows) parameter is required due to the trainer-sampler dependency. If the user specifies the wrong number of rows, then an error will occur showing what this parameter should be set to. Below is an example using [leave-one-out](LeaveOneOut.md) cross validation.

!listing cv_loo.i block=Samplers Trainers Surrogates CrossValidation

!syntax list /CrossValidation objects=True actions=False subsystems=False

!syntax list /CrossValidation objects=False actions=False subsystems=True

!syntax list /CrossValidation objects=False actions=True subsystems=False
