from signac import *

# load data
ds = Dataset()

fields = ['ZMass', 'ZPt', 'ZEta', 
    'M1Pt', 'M1Eta', 'M1Charge', 
    'M2Pt', 'M2Eta', 'M2Charge', 
    'MDr', 'Njets', 'Met', 'McType']
for f in fields: queueCommand("{0} = ds.addField('{0}', FieldType.Float, '{0}')".format(f))   

ds.open("C:/dev/omegalib/apps/signac-data/simdata.csv", CsvLoader())

f = Filter()

plots = []
selectionFilters = []

#-------------------------------------------------------------------------------
# Web API
import porthole
porthole.initialize(8080, 'signac/viewer/index.html')
ps = porthole.getService()

# client requests lists of available fields
def requestFieldList(client_id):
    data = []
    for f in fields: data.append({'label': f})
    ps.sendjs("receiveFieldList({0})"
        .format(data), client_id)

# client requests creation of a new plot
def requestNewPlot(client_id):
    plot = Plot()
    plot.setSize(400,400)
    plot.getBrush(0).setFilter(f)
    
    plot.setBrush(1, PlotBrush.create('selection'))
    plot.getBrush(1).setBlend(False)
    plot.getBrush(1).setEnabled(False)
    
    selectionFilters.append(Filter())
    
    plots.append(plot)
    ps.sendjs("createPlot({0}, {1}, {2})".format(len(plots) - 1, 400, 400), client_id)
        
def requestImage(plotId, width, height, client_id):
    plot = plots[plotId]
    plot.setSize(int(width), int(height))
    img = plot.getPixels()
    
    # If the pixels have not been updated, send an empty refresh
    if(not img.isDirty()):
        ps.sendjs("receiveImage({0}, 0, 0, '', '')"
                .format(plotId), client_id)
        return
    img.setDirty(False)
    
    b64img = porthole.base64EncodeImage(img, ImageFormat.FormatPng)
    
    if(plot.getX() != None and plot.getY() != None):
        info = {
            'xmin': plot.getX().getInfo().floatRangeMin,
            'xmax': plot.getX().getInfo().floatRangeMax,
            'ymin': plot.getY().getInfo().floatRangeMin,
            'ymax': plot.getY().getInfo().floatRangeMax,
            'xlabel': plot.getX().getInfo().label,
            'ylabel': plot.getY().getInfo().label
        }
    else:
        info = {
            'xmin': -1,
            'xmax': 1,
            'ymin': -1,
            'ymax': 1,
            'xlabel': 'X',
            'ylabel': 'Y'
        }
    
    ps.sendjs("receiveImage({0}, {1}, {2}, '{3}', {4})"
        .format(plotId, img.getWidth(), img.getHeight(), b64img, info), client_id)

        
def setFilter(filterId, field):
    print("setFilter {0}   {1}", filterId, field.getInfo().label)
    f.setField(filterId, field)
    
def setFilterRange(filterId, min, max):
    print("filter {0}   {1}".format(min, max))
    f.setNormalizedRange(filterId, float(min) / 100, float(max)/ 100)
    
def setSelection(plotid, minx, maxx, miny, maxy):
    f = selectionFilters[plotid]
    f.setField(0, plots[plotid].getX())
    f.setRange(0, minx, maxx)
    f.setField(1, plots[plotid].getY())
    f.setRange(1, miny, maxy)
    print('range x {0} -- {1}'.format(minx, maxx))
    print('range y {0} -- {1}'.format(miny, maxy))
 
def setBrushFilter(plotid, filterid):
    print('setting filter {0} to plot {1}'.format(filterid, plotid))
    b = plots[plotid].getBrush(1)
    b.setFilter(selectionFilters[filterid])
    b.setEnabled(True)
 