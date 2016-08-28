## Reference ##

--------------------------------------------------------------------------------
### DimensionType ###
Dimension types
- `Float`: dimension values are floating point type (single or double)

--------------------------------------------------------------------------------
### Dimension ###
A dimension represents a column in a source dataset.

#### id ####
> string id

The identifier of the dimension. This is used to retrieve the dimension using the dataset
loader. The dimension id is normally interpreted as a table name.

#### index ####
> int index

The index of the dimension. This is used to retrieve the dimension used the dataset loader.
The dimension index is normally interpreted as the column index in a table.

#### label ####
> string label

A descriptive label for the dimension.

#### dataset ####
> Dataset dataset

#### type ####
> [DimensionType](#dimensiontype) type 

--------------------------------------------------------------------------------
### Loader ###
Loader is the base class for data loaders

#### open ####
> open(string source)

Opens the specified source. Source depends on the specific loader implementation.

--------------------------------------------------------------------------------
