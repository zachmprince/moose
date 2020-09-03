# Creating a Surrogate Model

This example goes through the process of creating a custom surrogate model, in his case the creation of [NearestPointSurrogate.md].

## Overview

Building a surrogate model requires the creation of two objects: SurrogateTrainer and SurrogateModel. The SurrogateTrainer uses information from samplers and results to construct variables to be saved into a `.rd` file at the conclusion of the training run. The SurrogateMode object loads the data from the `.rd` and contains a function called `evaluate` that evaluates the surrogate model at a given input. The SurrogateTrainer and Surrogate are heavily tied together where each have the same member variables, the difference being one saves the data and the other loads it. It might be beneficial to have an interface class that contains common functions for training and evaluating, to avoid duplicate code. This example will not go into the creation of this interface class.

## Creating a Trainer

This example will go over the creation of [NearestPointTrainer](NearestPointTrainer.md). Most [trainers](Trainers/index.md) are derived from the [SamplerTrainer](SamplerTrainer.C) base class, which is technically a type of [GeneralUserObject.md] with the same `validParams`, `initialSetup`, `initialize`, `execute`, and `finalize` functions that are executed during the object's call. This class takes in predictor data in the form a sampler and response data in the form of a [vectorpostprocessor](VectorPostprocessors/index.md) vector, which can be seen in the following input parameters.

!listing SamplerTrainer.C re=InputParameters\sSamplerTrainer::validParams.*?^}

This parent class uses the `execute` function to loop through the sampler and it is the prerogative of the derived class, i.e. the trainer being created, to overwrite the `preTrain`, `train`, and `postTrain` functions.

!listing SamplerTrainer.C re=void\sSamplerTrainer::execute.*?^}

### Constructor

All trainers are based on [SurrogateTrainer](SurrogateTrainer.C), which provides the necessary interface for saving the surrogate model data. All the data meant to be saved is defined in the constructor of the training object. In [NearestPointTrainer](NearestPointTrainer.md), the variable `_sample_points` is declared as the necessary surrogate data, see [Trainers](Trainers/index.md) for more information on declaring model data:

!listing NearestPointTrainer.C re=NearestPointTrainer::NearestPointTrainer.*?^}

The member variable `_sample_points` is defined in the header file:

!listing NearestPointTrainer.h start=_sample_points end=_sample_points include-end=true

### preTrain

`preTrain` is called before the sampler loop. This is typically where the model data is resized based on the inputted predictor and response data. In [NearestPointTrainer.md], this is where the variable `_sampler_data` is resized based on the number of parameters and samples.

!listing NearestPointTrainer.C re=void\sNearestPointTrainer::preTrain.*?^}

### train

`train` is called within the local loop of samples. [SamplerTrainer.C] offers several protected member variables that the derived class can use for training, including `_row`, `_p`, `_local_p`, `_data`, and `_val`. `_row` is the global row index of the sampling matrix, while `_p` and `_local_p` are the global and local data index, respectively. `_data` is the current row of the sampler matrix (predictor data) and `_val` is value from the inputted vector (response data). [NearestPointTrainer.md] uses these to fill in the `_sampler_data` variable.

!listing NearestPointTrainer.C re=void\sNearestPointTrainer::train.*?^}


### postTrain

`postTrain` is called after the sampler loop is completed. This is typically where processor communication happens. Here, we use `postTrain` to gather all the local `_sample_points` so that each processor has the full copy. `_communicator.allgather` makes it so that every processor has a copy of the full array and `_communicator.gather` makes it so that only one of the processors has the full copy, the latter is typically used because outputting only happens on the root processor. See [libMesh::Parallel::Communicator](http://libmesh.github.io/doxygen/classlibMesh_1_1Parallel_1_1Communicator.html) for more communication options.

!listing NearestPointTrainer.C re=void\sNearestPointTrainer::postTrain.*?^}

## Creating a Surrogate

This example will go over the creation of [NearestPointSurrogate](NearestPointSurrogate.md). [Surrogates](Surrogates/index.md)  must have the `evaluate` public member function. The `validParams` for a surrogate will generally define how the surrogate is evaluated. [NearestPointSurrogate.md] does not have any options for the method of evaluation.

### Constructor

In the constructor, the references for the model data are defined, taken from the training data:

!listing NearestPointSurrogate.C re=NearestPointSurrogate::NearestPointSurrogate.*?^}

See [Surrogates](Surrogates/index.md) for more information on the `getModelData` function. `_sample_points` in the surrogate is a `const` reference, since we do not want to modify the training data during evaluation:

!listing NearestPointSurrogate.h start=/// Array end=sample_points include-end=true

### evaluate

`evaluate` is a public member function required for all surrogate models. This is where surrogate model is actually used. `evaluate` takes in parameter values and returns the surrogate's estimation of the quantity of interest. See [EvaluateSurrogate](EvaluateSurrogate.C) for an example on how the `evaluate` function is used.

!listing NearestPointSurrogate.C re=Real\sNearestPointSurrogate::evaluate.*?^}
