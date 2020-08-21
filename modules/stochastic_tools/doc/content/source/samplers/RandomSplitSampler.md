# RandomSplit

!syntax description /Samplers/RandomSplit

## Overview

This sampler randomly extracts rows from a supplied sampler. The number of rows to extract is set by [!param](/Samplers/RandomSplit/num_rows). By default, no row is repeated which means [!param](/Samplers/RandomSplit/num_rows) must be less than the number of rows in the provided sampler. Rows can be repeated if [!param](/Samplers/RandomSplit/allow_repeated_rows) is set to `true`, which means [!param](/Samplers/RandomSplit/num_rows) can be any number greater than 0.

## Example Input Syntax

In the following input, `split` will selects 9 rows from `sample` that are in a random order and `split_repeat` extracts 15 random rows with some repeated.

!listing random_split.i block=Samplers

!syntax parameters /Samplers/RandomSplit

!syntax inputs /Samplers/RandomSplit

!syntax children /Samplers/RandomSplit
