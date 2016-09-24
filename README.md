## Reference ##

--------------------------------------------------------------------------------
### DimensionType ###
Dimension types
- `Float`: dimension values are floating point type (single or double)

--------------------------------------------------------------------------------
### Dimension ###
A dimension represents a column in a source dataset. Dimension variables are often passed to shaders
to access a particular column of data for a point.

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

A pointer variable representing the Dataset instance that the Dimension variable is tied to.

#### type ####
> [DimensionType] type 

Describes the type of data that is stored in the column as a DimensionType. 
--------------------------------------------------------------------------------
### Field ###

A Field specifies a subset of a dimension which is limited to a certain domain of data.

#### getDimension ####
> [Dimension](#dimension) getDimension()

Returns a pointer to the Dimension which the Field is a part of.

#### loaded ####
> bool loaded()
Specifies whether the current domain of data is currently in memory. Domains are loaded before the memory is needed for visualization, as opposed to when the dimension is specified.

#### range ####
> [Range] range()

--------------------------------------------------------------------------------
### Loader ###
Loader is the base class for data loaders

#### open ####
> open(string source)

Opens the specified source. Source depends on the specific loader implementation.

--------------------------------------------------------------------------------
### CsvLoader ###
> extends [Loader]

An extention of loader used to open simple CSV files.

--------------------------------------------------------------------------------
### NumpyLoader ###
> extends [Loader]

An extention of loader used to load data from numpy arrays.

#### addDimension ####
> addDimension(string name, nparray data)

--------------------------------------------------------------------------------
### Hdf5Loader ###
> extends [Loader]

An extention of loader used to open simple hdf5 format files

--------------------------------------------------------------------------------
### FireLoader ###
> extends [Loader]

Variation of the hdf5loader with support for loading from multi-part files.

--------------------------------------------------------------------------------
### BinaryLoader ###
> extends [Loader]

--------------------------------------------------------------------------------
### Dataset ###

Datasets are linked to an HDF5 particle set and assist in accessing data loaded from the respective loading module.

#### create ####
> static [Dataset] create()

Creates a new Dataset object.

#### setLoader ####
> setLoader([Loader] loader)

Links the Dataset to a file loader, which the dataset will use to pull data from.

#### addField ####
> addField([Field] field)

Adds a field of data that the dataset will attempt to load from using it's respective loader.

#### addDimension ####
> addDimension([Dimension] dimension)

Defines a Dimension class object, or data source column.

#### useDoublePrecision ####

Returns a boolean indicating whether the data loaded through the dataset will be in double precision or single precision.

#### setDoublePrecision ####
> setDoublePrecision(bool enabled)
> bool useDoublePrecision()

Sets whether the data loaded through the dataset will use double precision or single precision floating point values.

--------------------------------------------------------------------------------
### Filter ###

#### setField ####
> setField([Field] field)

#### setRange ####
> setRange(float min, float max)

#### setNormalizedRange ####
> setNormalizedRange(float min, float max)

--------------------------------------------------------------------------------
### PlotBrush ###

#### create ####
> static [PlotBrush] create()

#### setFilter ####
> setFilter([Filter]) filter

#### setColor ####
> setColor([Color] color)

#### setEnabled ####
#### isEnabled ####
> setEnabled(bool enabled)
> bool isEnabled()

#### refresh ####
> refresh()

#### setBlend ####
> setBlend(bool enabled)

--------------------------------------------------------------------------------
### Plot ###

#### setX ####
#### setY ####
#### getX ####
#### getY ####
> setX([Field] x)

> [Field] getX()

> setY([Field] y)

> [Field] getY()

#### setSize ####
> setSize(int width, int height)

#### refresh ####
> refresh()

#### getPixels ####
> [PixelData] getPixels()

#### getBrush ####
#### setBrush ####
> [PlotBrush] getBrush(int id)
> setBrush(int id, [PlotBrush])

--------------------------------------------------------------------------------
### PointCloudView ###

#### addPointCloud ####
> addPointCloud([PointCloud] pc)

#### removePointCloud ####
> removePointCloud([PointCloud] pc)

#### resize ####
> resize(int width, int height)

#### enableColormapper ####
> enableColormapper(bool enabled)

#### setColormapper ####
> setColormapper([Program] colormapper)

#### setColormap ####
> setColormap([PixelData] colormap)

#### updateChannelBounds ####
> updateChannelBounds(bool useChannelTexture)

#### setChannelBounds ####
#### getChannelMin ####
#### getChannelMax ####
> setChannelBounds(float channelMin, float channelMax)

> float getChannelMin()

> float getChannelMax()

#### getOutput ####
> [PixelData] getOutput

--------------------------------------------------------------------------------
### PointCloud ###

> extends [NodeComponent]

A set of scattered points that are drawn when assigned to a Scene Node. Displays the points sent to it from a Dataset.

#### create ####
> static [PointCloud] create()

Creates and returns a new pointCloud object

#### setOptions ####
> setOptions(string options)

Sets the Decimation level of the PointCloud. Decimation is used to lower the amount of points loaded for very large data sets, and instead load 1 point for every decimation value. The settings here modify the Loader decimation value. The input is a string should be in the format: "50000 0:100000:X" where X is the decimation level for the loader. The true decimation level is the product of the loader decimation level and the display decimation level. Higher decimation levels result in fewer points being loaded.

#### setDimensions ####
> setDimensions([Dimension] x, [Dimension] y, [Dimension] z)

Specifies the dimensions that every point loaded will use to determine it's x, y, and z positions when plotted. 

#### setData ####
> setData([Dimension] data)

Specifies an additional dimension of data that the pointCloud object will send to the shader, that can be used for custom rendering options.

#### setVectorData ####
> setVectorData([Dimension] vx, [Dimension] vy, [Dimension] vz)

Used to send three dimensions to the pointCloud's geometry shader. Intended to be used to create 
#### getDataX ####
#### getDataY ####
#### getDataZ ####

> [Dimension] getDataX()
> [Dimension] getDataY()
> [Dimension] getDataZ()

Used to retrieve vector data variables.

#### setSize ####
> setSize([Dimension] size)

Sends an additional dimension of data to the vertex shader for every point. Intended to be used to specify the vertex's pointSize.

#### setFilter ####
> setFilter([Dimension] filter)

Specifies the dimension that is used for filtering.

#### setFilterBounds ####
> setFilterBounds(float min, float max)

Specifies the bounds of the current PointCloud filter dimension. Objects beyond the filter's dimensions for the current filterDimension will nto be drawn.

#### normalizeFilterBounds ####
> normalizeFilterBounds(bool normalize)

Specifies whether the min and max filters are on a normalized or absolute scale. On a normalized scale, 1.0 represents the highest value of the current Dimension in the pointCLoud's data set, while 0.0 represents the lowest value.

#### setProgram ####
> setProgram([Program] program)

Sets the shader Program that the pointCloud will use. The Program will in turn determine the Vertex, Geometry, and Fragment shaders that will be used when the PointCloud object is drawn.

#### getProgram ####
> [Program] getProgram()

Returns a reference to the current program that the pointCloud is using.

#### setPointScale ####
> setPointScale(float scale)

Sets the pointScale uniform that is sent to the pointCloud's shaders. Intended to be used when all of the points in a particular pointCloud are intended to have the same size.

#### setColormap ####
#### getColormap ####
> setColormap([PixelData] colormap)

Sets the colormap object associated with this program.

> [PixelData] getColormap()

#### setColor ####
#### getColor ####
> setColor([Color] color)

Sends a uniform to the pointCloud's shaders intended to represent a uniform color for all shaders.

> [Color] getColor()

Returns the current color.

#### setDecimation ####
> setDecimation(int decimation)

Sets the dynamic decimation for the pointCloud. When dynamic decimation is set, ony one point for every set of the decimation value is displayed, reducing GPU strain. The dynamic decimation is layered on top of the loader decimation. The true displayed decimation is a product of the two.

#### setFocusPosition ####
> setFocusPosition([Vector3] pos)

Passes the parameter 'focusPosition' to the Program Shaders.

#### setVisible ####
> setVisible(bool visible)

Sets whether the pointCloud object should be drawn.
--------------------------------------------------------------------------------
### Program ###
A collection of shaders which is sent to Signac and used on point data. A program can later be set from a PointCloud, which will make all three shaders associated with the program the active shaders when drawing the particular point cloud.

#### setVertexShader ####
> setVertexShader(string file)

Specifies the Vertex shader associated with this program. Vertex Shaders are called for every point in Signac.

#### setFragmentShader ####
> setFragmentShader(string file)

Specifies the fragment shader associated with this program. Fragment shaders will be called for every pixel on the monitor screen and are used when created a 2d texture from the 3d point data.

#### setGeometryShader ####
> setGeometryShader(string file)

Specifies the Geometry shader associated with this program. The Geometry shaders take a single primitive point and can treat the point as a more complicated geometric shape. Useful for multiple-layer rendering.

#### reload ####
> reload()

Forces the shaders to update file sources and defined variables.

#### define ####
> define(string definename, string definevalue)

Sets a shader variable of a certain name to a value. The next time a shader is called, it will have the variable of the defined name as the specified value. This can be used to set flags.

#### clearDefines ####
> clearDefines

Removes all previously defined variables, removing all keys and values.

--------------------------------------------------------------------------------
### Signac ###

The base module for Signac.

#### getInstance ####
> string [Signac] getInstance()

Returns an instance of signac

#### addProgram ####
> addprogram([Program] prog)

Loads a Shader Program into signac, which can later be actived and deactivated dynamically.

#### setFieldLoadedCommand ####
> setFieldLoadedCommand(string command)

When called from python, sets a python function that is called after signac is finished loading all data for all current datasets.

#### setWorkerThreads ####
> setWorkerThreads(int threads)

Links Signac to additional worker threads which are used for loading data points from multiple datasets.

[Filter]: #filter
[Dimension]: #dimension
[DimensionType]: #dimensiontype
[Loader]: #loader
[Dataset]: #dataset
[Field]: #field
[PlotBrush]: #plotbrush
[PointCloud]: #pointcloud
[Program]: #program
[Signac]: #signac
[NodeComponent]: https://github.com/uic-evl/omegalib/wiki/SceneNode
[Vector3]: https://github.com/uic-evl/omegalib/wiki/Vector
[Color]: https://github.com/uic-evl/omegalib/wiki/Color
[PixelData]: https://github.com/uic-evl/omegalib/wiki/PixelData
