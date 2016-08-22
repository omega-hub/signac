#include "signac.h"
#include "PointCloud.h"
#include "Loader.h"

///////////////////////////////////////////////////////////////////////////////
PointCloud::PointCloud(const String& name) : NodeComponent(),
myVisible(true)
{
    float maxf = numeric_limits<float>::max();
    float minf = -numeric_limits<float>::max();
    //myBBox.setExtents(Vector3f(minf, minf, minf), Vector3f(maxf, maxf, maxf));
    myMinDataBounds = Vector4f(maxf, maxf, maxf, maxf);
    myMaxDataBounds = Vector4f(minf, minf, minf, minf);
    myProgramParams = new ProgramParams();

    myBatchDrawStat = Stat::create(ostr("%1% batches", %name), StatsManager::Primitive);
}

///////////////////////////////////////////////////////////////////////////////
bool PointCloud::setOptions(const String& options)
{
    LOD::parse(options, &myLODLevels, &myPointsPerBatch);
    return true;
}

///////////////////////////////////////////////////////////////////////////////
bool PointCloud::setDimensions(Dimension* x, Dimension* y, Dimension* z)
{
    myX = x;
    myY = y;
    myZ = z;

    // Dimensions must all be from the same dataset
    if(myX->dataset != myY->dataset || 
        myX->dataset != myZ->dataset || 
        myY->dataset != myZ->dataset) 
    {
        owarn("PointCloud dimensions must all be from the same dataset");
        return false;
    }

    myDataset = myX->dataset;

    size_t numRecords = myDataset->getNumRecords();
    if(numRecords == 0) return false;

    ofmsg("[BinaryPointsLoader] Total Points: <%1%>   Points per batch: <%2%>",
        %numRecords
        %myPointsPerBatch);

    // Iterate for each batch
    for(size_t start = 0; start <= numRecords; start += myPointsPerBatch)
    {
        PointBatch* batch = new PointBatch(this);
        myBatches.push_back(batch);

        // Create LOD groups for each batch
        foreach(LOD& ll, myLODLevels)
        {
            batch->addDrawable(&ll, start, myPointsPerBatch);
        }

        // Batch bounding box is not ready at this point.
        //myBBox.merge(batch->getBoundingBox());
    }

    refreshFields();

    return true;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setVectorData(Dimension* x, Dimension* y, Dimension* z)
{
    myDataX = x;
    myDataY = y;
    myDataZ = z;
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setData(Dimension* fi)
{ 
    myData = fi; 
    refreshFields(); 
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setSize(Dimension* fi) 
{ 
    mySize = fi; 
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFilter(Dimension* fi) 
{ 
    myFilter = fi; 
    refreshFields(); 
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::refreshFields()
{
    foreach(PointBatch* b, myBatches) b->refreshFields();
    if(myProgram != NULL)
    {
        if(mySize != NULL) myProgram->define("SIZE_MODE", "1");
        else myProgram->define("SIZE_MODE", "0");
        if(myFilter != NULL) myProgram->define("FILTER_MODE", "1");
        else myProgram->define("FILTER_MODE", "0");
        if(myData != NULL) myProgram->define("DATA_MODE", "1");
        else myProgram->define("DATA_MODE", "0");
    }

    myBBox.setNull();
    if(getOwner() != NULL)
    {
        foreach(PointBatch* b, myBatches)
        {
            if(b->hasBoundingBox())
            {
                // Batch bounding box is not ready at this point.
                myBBox.merge(b->getBoundingBox());
            }
            getOwner()->requestBoundingBoxUpdate();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::draw(const DrawContext& c)
{
    if(!isVisible()) return;
    myProgram->setParams(c, myProgramParams);

    Rectf viewRect(-1, -1, 2, 2);

    int bc = 0;
    foreach(PointBatch* b, myBatches)
    {
        if(b->hasBoundingBox())
        {
            // The screen culling is not working correctly...
            /*const AlignedBox3& bb = b->getBoundingBox();
            const Vector3f* corners = bb.getAllCorners();
            Transform3 xf = c.projection * c.modelview * getOwner()->getFullTransform();
            Vector2f vmin(2.0f, 2.0f);
            Vector2f vmax(-2.0f, -2.0f);
            for(int i = 0; i < 8; i++)
            {
                Vector4f ch(corners[i][0], corners[i][1], corners[i][2], 1.0f);
                Vector4f c = xf * ch;
                c = c / c.w();
                vmin = vmin.cwiseMin(Vector2f(c[0], c[1]));
                vmax = vmax.cwiseMax(Vector2f(c[0], c[1]));
            }
            float x = vmin[0];
            float y = vmin[1];
            float w = vmax[0] - vmin[0];
            float h = vmax[1] - vmin[1];
            Rectf br(x, y, w, h);

            if(br.intersects(viewRect))*/
            {
                b->draw(c);
                bc++;
            }
        }
        else
        {
            // If the batch drawable does not have a bounding box yet, just
            // draw it. Bounding box will be ready eventually.
            b->draw(c);
            bc++;
        }
    }

    myBatchDrawStat->addSample(bc);
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setProgram(Program* p)
{
    myProgram = p;
    refreshFields();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::update(const UpdateContext& ctx)
{
    SceneNode* sn = getOwner();
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setPointScale(float scale)
{
    myProgramParams->pointScale = scale;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFilterBounds(float fmin, float fmax)
{
    myProgramParams->filterMax = fmax;
    myProgramParams->filterMin = fmin;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::normalizeFilterBounds(bool enabled)
{
    myProgramParams->normalizedFilterBounds = enabled;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setColor(const Color& c)
{
    myProgramParams->color = c;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setFocusPosition(const Vector3f d)
{
    myProgramParams->focusPosition = d;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setDecimation(int dec)
{
    myProgramParams->decimation = dec;
}

///////////////////////////////////////////////////////////////////////////////
void PointCloud::setColormap(PixelData* colormap)
{
    myColormap = colormap;
}
