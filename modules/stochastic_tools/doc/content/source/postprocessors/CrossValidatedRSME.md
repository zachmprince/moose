# CrossValidatedRSME

!syntax description /Postprocessors/CrossValidatedRSME

## Overview

This postprocessor computes the cross validated root-mean-squared-error (RMSE) for a given set of [surrogate models](Surrogates/index.md) and testing sets:

!equation
\mathrm{CV}_{\mathrm{RMSE}} = \sqrt{\frac{\sum_{k=1}^K\frac{\sum_{i=1}^{N_k}\left(y_{i_k}-\hat{y}_{k,i_k}\right)^2}{N_k}}{K}} ,

where $K$ is the number of test/model sets, $N_k$ is the number of point in model $k$, $y_{i}$ is the response value at data point $i$, $\hat{y}_{k,i}$ is the $k^{\mathrm{th}}$ model's predicted response for data point $i$, and $i_k$ is the data point index of test point $i$ in set $k$.

This postprocessor is used by the [cross validation system](CrossValidation/index.md) and is setup automatically.

The observed data ($\vec{y}$) is inputted as a [vector postprocessor](VectorPostprocessors/index.md) vector using the [!param](/Postprocessors/CrossValidatedRSME/data_vpp) and [!param](/Postprocessors/CrossValidatedRSME/data_vector) parameters. The predictor data is inputted as a [sampler](Samplers/index.md) using the [!param](/Postprocessors/CrossValidatedRSME/sampler) parameter. The models are specified in the [!param](/Postprocessors/CrossValidatedRSME/models) parameter. The [!param](/Postprocessors/CrossValidatedRSME/test_index) and [!param](/Postprocessors/CrossValidatedRSME/num_points) should contain a vector of values for +each+ model inputted. [!param](/Postprocessors/CrossValidatedRSME/test_index) specifies the first index in a chunk of data and the corresponding [!param](/Postprocessors/CrossValidatedRSME/num_points) specifies the number of points to include after that index.

## Example Input File Syntax

Below is an example of using this postprocessor on a [PolynomialRegressionSurrogate.md].

!listing cv_pp.i block=Postprocessors Trainers Surrogates

Here `poly_reg1` is tested with data points/sampler row 101--200 and `poly_reg2` is tested with 1--100.

!syntax parameters /Postprocessors/CrossValidatedRSME

!syntax inputs /Postprocessors/CrossValidatedRSME

!syntax children /Postprocessors/CrossValidatedRSME
