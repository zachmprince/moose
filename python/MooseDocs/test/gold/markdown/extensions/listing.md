# Listing Extension {#listing-extension}

## Test Numbering {#test-numbering}

[Listing 1: ]{#one}

``` 
One
```

[Listing 2: ]{#two}

``` 
Two
```

[Listing 3: ]{#three}

``` 
Three
```

You can reference listings: [Listing 3](#three).

## Test Captions {#test-captions}

``` 
no caption
```

the **caption**

``` 
caption with inline content
```

[File 1: the [caption]{.underline}]{#file1}

``` 
caption with prefix and number
```

Reference to special prefix: [File 1](#file1).

## Listing with space {#listing-with-space}

``` 
void function();

void anotherFunction();
```

### Test Limited Height {#test-limited-height}

The container for this listing is limited to 92 pixels, vertically:

``` 
#include <stdio.h>

void
greeting(void)
{
  printf("Hello, World!\n");
}

int
main(int argc, char **argv)
{
  greeting();
}
```

## Language {#language}

``` c++
void function();
```

## File Listings {#file-listings}

Display a C++ source file (excluding the MOOSE header) and hide the
modal link:

[Listing 4: ]{#diffusion-c}

``` cpp
#include "Diffusion.h"

registerMooseObject("MooseApp", Diffusion);

InputParameters
Diffusion::validParams()
{
  InputParameters params = Kernel::validParams();
  params.addClassDescription("The Laplacian operator ($-\\nabla \\cdot \\nabla u$), with the weak "
                             "form of $(\\nabla \\phi_i, \\nabla u_h)$.");
  return params;
}

Diffusion::Diffusion(const InputParameters & parameters) : Kernel(parameters) {}

Real
Diffusion::computeQpResidual()
{
  return _grad_u[_qp] * _grad_test[_i][_qp];
}

Real
Diffusion::computeQpJacobian()
{
  return _grad_phi[_j][_qp] * _grad_test[_i][_qp];
}
```

Display only the `validParams()` definition and limit height to 92
pixels, vertically:

``` cpp
InputParameters
Diffusion::validParams()
{
  InputParameters params = Kernel::validParams();
  params.addClassDescription("The Laplacian operator ($-\\nabla \\cdot \\nabla u$), with the weak "
                             "form of $(\\nabla \\phi_i, \\nabla u_h)$.");
  return params;
}
```

(framework/src/kernels/Diffusion.C)

### HIT Files {#hit-files}

Extract the `[Mesh]` and `[Kernels]` blocks and use a special prefix:

[xxxxx 1: ]{#prfx}

``` moose
[Mesh]
  type = GeneratedMesh
  dim = 2
  nx = 10
  ny = 10
[]

[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
[]
```

(test/tests/kernels/simple_diffusion/simple_diffusion.i)

Extract the `[diff]` block, indent 2 spaces, and add `[AuxKernels]` and
`[]` as header and footer:

``` moose
[Kernels]
  [diff]
    type = Diffusion
    variable = u
  []
[]
```

(test/tests/kernels/simple_diffusion/simple_diffusion.i)

Remove the `[test]` block and hide the modal link:

``` text
[Tests]
[]
```

Remove the `issues` and `design` parameters:

``` text
[Tests]
  [test]
    type = 'Exodiff'
    input = 'simple_diffusion.i'
    exodiff = 'simple_diffusion_out.e'

    requirement = 'The system shall run a simple 2D linear diffusion problem with Dirichlet boundary conditions on a regular mesh.'

    # Enables running the limited HPC tests on CIVET on all events
    group = 'hpc'
  []
[]
```

(test/tests/kernels/simple_diffusion/tests)

Extract the `[BCs]` block then remove the `[left]` block and the `value`
parameter:

``` moose
[BCs]
  [right]
    type = DirichletBC
    variable = u
    boundary = right
  []
[]
```

(test/tests/kernels/simple_diffusion/simple_diffusion.i)