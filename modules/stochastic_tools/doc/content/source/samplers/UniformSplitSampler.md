# UniformSplit

!syntax description /Samplers/UniformSplit

## Overview

This sampler extracts uniform portions of another sampler given a set of initial rows ([!param](/Samplers/UniformSplit/sampler_row)) and number of rows ([!param](/Samplers/UniformSplit/num_rows)).

## Example Input Syntax

In the following input, the UniformSplit sampler extracts rows 1, 2, 3, 5, 6, and 10.

!listing uniform_split.i block=Samplers

!syntax parameters /Samplers/UniformSplit

!syntax inputs /Samplers/UniformSplit

!syntax children /Samplers/UniformSplit
