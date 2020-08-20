# Goodness of Fit

!syntax description /VectorPostprocessors/GoodnessOfFit

## Overview

This object provides the capability to calculate [goodness of fit](https://en.wikipedia.org/wiki/Goodness_of_fit) metrics for surrogate models. The idea is that one can use these metrics to determine the most appropriate surrogate model to use after training and determine how well each model fits the provided data. The metrics in this object are meant to treat the surrogate models as black boxes, simply comparing the observed (reference) data and model data from evaluating the surrogate model.

The observed data is taken directly from a vector postprocessor, usually from a [StochasticResults.md] VPP. The parameters that +must+ be specified for this are [!param](/VectorPostprocessors/GoodnessOfFit/data_vpp) and [!param](/VectorPostprocessors/GoodnessOfFit/data_vector). There are two ways a user can provide the model data. The first to directly specify a vector(s) from VPP(s) that contain the evaluation of the surrogate models, usually from a [EvaluateSurrogate.md] VPP. The parameters that +must+ be specified for this are [!param](/VectorPostprocessors/GoodnessOfFit/model_results_vpp) and [!param](/VectorPostprocessors/GoodnessOfFit/model_results_vector), the number of items in each of these parameters must also match (one for each model). The second way is to use a sampler to evaluate the surrogate object. The parameters that +must+ be specified for this are [!param](/VectorPostprocessors/GoodnessOfFit/model) and [!param](/VectorPostprocessors/GoodnessOfFit/sampler).

There are a total five metrics that can be computed, specified through the [!param](/VectorPostprocessors/GoodnessOfFit/compute) parameter, which are described in the following sections. Some metrics require that the models be specified in the [!param](/VectorPostprocessors/GoodnessOfFit/model) parameter, this is to access the number of degrees of freedom in the model, these metrics include:

- Adjusted $R^2$ --- `adj-rsquared`
- F-statistic --- `fstatistic`
- $p$-Value --- `pvalue`

### Root Mean Square Error

Having `rmse` in [!param](/VectorPostprocessors/GoodnessOfFit/compute) will compute the [root mean square error (RMSE)](https://en.wikipedia.org/wiki/Root-mean-square_deviation) of the model data. RMSE is defined as:

!equation
\text{RMSE} = \sqrt{\frac{\sum_{i=1}^{n}(\hat{y}_i - y_i)^2}{n}},

where $n$ is the number of data points, $\hat{y}$ is the model data and $y$ is the observed data.

### Coefficient of Determination

Having `rsquared` in [!param](/VectorPostprocessors/GoodnessOfFit/compute) will compute the [$R^2$ value](https://en.wikipedia.org/wiki/Coefficient_of_determination) of the model data. $R^2$ is defined as:

!equation
R^2 = 1 - \frac{\sum_{i=1}^{n}(\hat{y}_i - y_i)^2}{\sigma^2}

where $\sigma$ is the standard deviation of the observed data.

Having `adj-rsquared` in [!param](/VectorPostprocessors/GoodnessOfFit/compute) will compute the [adjusted $R^2$ value](https://en.wikipedia.org/wiki/Coefficient_of_determination#Adjusted_R2) of the model data. Adjusted $R^2$ is defined as:

!equation
\bar{R}^2 = 1 - (1-R^2)\frac{n-1}{n-\nu-1}

where $\nu$ is the number of degrees of freedom in the model. For example, in [PolynomialRegressionSurrogate.md], $\nu$ is equivalent to the number coefficients.

### Significance of Regression

The significance of regression metrics in this object involves the [F-test](https://en.wikipedia.org/wiki/F-test). This test is sensitive to the non-normality of residuals, which basically means how symmetric the observed data fits around the model data, which can be useful for determining biases and over-fitting. Two options in the [!param](/VectorPostprocessors/GoodnessOfFit/compute) parameter are available to perfrom an F-test: `fstatistic` and `pvalue`. `fstatistic` computes the F-statistic defined by,

!equation
F = \frac{\sum_i^n (\hat{y} - \bar{y})^2/(p-1)}{\sum_i^n (\hat{y} - y_i)^2/(n-\nu)} = \frac{R^2}{1-R^2}\frac{n-\nu}{\nu-1}.

The F-test assumes this statistic follows an [FDistribution.md], so the `pvalue` is computed by determining the probability that the statistic is greater than the one computed, i.e,

!equation
P(F_{\nu-1, n-\nu} > F) = 1 - F(F; \nu-1, n-\nu),

where $F(x; d_1, d_2)$ is the cumulative distribution function defined in [FDistribution.md].

## Example Input File Syntax

Firstly, the input file must instantiate a surrogate model either by inputting a trainer object or trainer data file:

!listing gof_sampler.i block=Surrogates

The GoodnessOfFit object can then be defined by either inputting vectors of model data:

!listing gof.i block=VectorPostprocessors

Or by inputting models and a sampler:

!listing gof_sampler.i block=VectorPostprocessors

!syntax parameters /VectorPostprocessors/GoodnessOfFit

!syntax inputs /VectorPostprocessors/GoodnessOfFit

!syntax children /VectorPostprocessors/GoodnessOfFit
