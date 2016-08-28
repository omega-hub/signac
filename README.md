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
> [DimensionType] type 

--------------------------------------------------------------------------------
### Field ###

#### getDimension ####
> [Dimension](#dimension) getDimension()

#### loaded ####
> bool loaded()

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

--------------------------------------------------------------------------------
### Hdf5Loader ###
> extends [Loader]

--------------------------------------------------------------------------------
### FireLoader ###
> extends [Loader]

--------------------------------------------------------------------------------
### BinaryLoader ###
> extends [Loader]

--------------------------------------------------------------------------------
### Dataset ###

#### create ####
> static [Dataset] create()

#### setLoader ####
> setLoader([Loader] loader)

#### addField ####
> addField([Field] field)

#### addDimension ####
> addDimension([Dimension] dimension)

#### useDoublePrecision ####
#### setDoublePrecision ####
> setDoublePrecision(bool enabled)
> bool useDoublePrecision()

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

#### create ####
> static [PointCloud] create()

#### setOptions ####
> setOptions(string options)

#### setDimensions ####
> setDimensions([Dimension] x, [Dimension] y, [Dimension] z)

#### setData ####
#### getDataX ####
#### getDataY ####
#### getDataZ ####
> setData([Dimension] data)

> [Dimension] getDataX()

> [Dimension] getDataY()

> [Dimension] getDataZ()

#### setVectorData ####
> setVectorData([Dimension] vx, [Dimension] vy, [Dimension] vz)

#### setSize ####
> setSize([Dimension] size)

#### setFilter ####
> setFilter([Dimension] filter)

#### setFilterBounds ####
> setFilterBounds(float min, float max)

#### normalizeFilterBounds ####
> normalizeFilterBounds(bool normalize)

#### setProgram ####
#### getProgram ####
> setProgram([Program] program)

> [Program] getProgram()

#### setPointScale ####
> setPointScale(float scale)

#### setColormap ####
#### getColormap ####
> setColormap([PixelData] colormap)

> [PixelData] getColormap()

#### setColor ####
#### getColor ####
> setColor([Color] color)

> [Color] getColor()

#### setDecimation ####
> setDecimation(int decimation)

#### setFocusPosition ####
> setFocusPosition([Vector3] pos)

#### setVisible ####
> setVisible(bool visible)

--------------------------------------------------------------------------------
### Program ###

#### setVertexShader ####
> setVertexShader(string file)

#### setFragmentShader ####
> setFragmentShader(string file)

#### setGeometryShader ####
> setGeometryShader(string file)

#### reload ####
> reload()

#### define ####
> define(string definename, string definevalue)

#### clearDefines ####
> clearDefines

--------------------------------------------------------------------------------
### Signac ###

#### getInstance ####
> string [Signac] getInstance()

#### addProgram ####
> addprogram([Program] prog)

#### setFieldLoadedCommand ####
> setFieldLoadedCommand(string command)

#### setWorkerThreads ####
> setWorkerThreads(int threads)

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
