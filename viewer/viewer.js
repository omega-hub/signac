plots = {}

//------------------------------------------------------------------------------
// create the ui
$(document).ready(function() {
    // Create the toolbar
    $("#toolbar").jqxToolBar({
        width: "100%", height: 35, tools: "button",
        initTools: function(type, index, tool, menuToolInitialization) {
            switch(index) {
                case 0:
                    tool.jqxButton();
                    tool.text("New Plot");
                    tool.on("click", function(e) {
                        porthole.call("requestNewPlot(%client_id%)")
                    })
            }
        }
    })
    
    // Create the docking layout
    var layout = [{
        type: 'layoutGroup',
        orientation: 'horizontal',
        width: "100%",
        items: [{
            type: 'layoutGroup',
            orientation: 'vertical',
            alignment: 'left',
            width: '20%',
            items: [{
                type: 'tabbedGroup',
                height: '50%',
                items: [{
                    type: 'layoutPanel',
                    title: 'Fields',
                    contentContainer: 'FieldsPanel',
                }]
            },{
                type: 'tabbedGroup',
                height: '50%',
                items: [{
                    type: 'layoutPanel',
                    title: 'Filters',
                    contentContainer: 'FiltersPanel',
                }]
            }]
        },{
            type: 'documentGroup',
            width: '80%',
            items: [{
                type: 'documentPanel',
                title: 'Plots',
                contentContainer: 'PlotPanel',
            }]
        }]
    }]
    $('#layout').jqxLayout({layout: layout});
    
    // Enable docking in plot area
    //$('#PlotPanel').jqxDocking({width: "50%", height:"100%"});
})

function connected() {
    //porthole.call("requestImage('plot1', %client_id%)")
    //porthole.call("requestImage('plot2', %client_id%)")
    porthole.call("requestFieldList(%client_id%)")
    createFilters();
}
window.setTimeout(connected, 1000);

//----------------------------------------------------------------------
// Create the filter UI
function createFilters() {
    // Select the side panel content
    sidebar = d3.select('#FiltersPanel');
    
    filters = ['filter1', 'filter2', 'filter3', 'filter4'];
    
    filterElements = sidebar.selectAll('div')
        .data(filters)
        .enter()
        .append('div')
            .attr('class', 'filter')
            .append('div')
                .attr('class', 'slot')
                .text('Filter field')
                .on('dragover', function(d) {
                    d3.event.preventDefault();
                    d3.event.dataTransfer.dropEffect = 'copy';
                    return false;
                })
                .on('drop', function(d, i) {
                    args = d3.event.dataTransfer.getData('Text').split(' ');
                    if(args[0] === 'field') {
                        fieldName = args[1];
                        setFilter(d, i, fieldName);
                        d3.select(this).text(fieldName);
                    }
                })
            .select(function() { return this.parentNode})
            .append('div')
                .attr('id', function(d) {return d; })

    filters.forEach(function(d, i) {
        
        $('#'+ d).jqxSlider({
            showButtons: true, 
            rangeSlider: true,
            showRange: true,
            tooltip: true,
            mode: 'fixed',
            min: 0,
            max: 100,
            values: [0, 100],
            width: "90%",
            height: 20
        })
        .css('padding-top', '20px')
        .on('slide', function(event) {
            v = event.args.value;
            porthole.call("setFilterRange({0}, {1}, {2})", i, v.rangeStart, v.rangeEnd)
        });
    });  
}

//------------------------------------------------------------------------------
function createPlot(plotId, width, height) {
    $('#PlotPanel').append("<div id='plot" + plotId + "'><div>Plot " + plotId + 
    "</div><div id='contentDiv'><div id='plotSlots'></div></div></div>");
    
    marginx = 20;
    marginy = 120;
    
    plot = $('#plot' + plotId);
    plot.css("width", (width + marginx) + "px");
    plot.css("height", (height + marginy) + "px");
    
    $('#PlotPanel').jqxDocking('addWindow', 'plot' + plotId);
    
    $('#plot' + plotId).jqxWindow({showCollapseButton: true});
    
    porthole.call("requestImage({0}, 400,400, %client_id%)", plotId);
    
    plotSlots = d3.select('#plot' + plotId + ' #contentDiv #plotSlots');
    plotSlots.append("div")
        .attr('class', 'slot')
        .text("X")
        .on('dragover', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            d3.event.dataTransfer.dropEffect = 'copy';
            return false;
        })
        .on('drop', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            args = d3.event.dataTransfer.getData('Text').split(' ');
            if(args[0] === 'field') {
                fieldName = args[1]
                porthole.call('plots[{0}].setX(' + fieldName + ')', plotId)
                d3.select(this).text(fieldName);
            }            
        });
        
    plotSlots.append("div")
        .attr('class', 'slot')
        .text("Y")
        .on('dragover', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            d3.event.dataTransfer.dropEffect = 'copy';
            return false;
        })
        .on('drop', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            args = d3.event.dataTransfer.getData('Text').split(' ');
            if(args[0] === 'field') {
                fieldName = args[1]
                porthole.call('plots[{0}].setY(' + fieldName + ')', plotId)
                d3.select(this).text(fieldName);
            }            
        });   
        
    plotSlots.append("div")
        .attr('class', 'slot')
        .text("Selection")
        .on('dragover', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            d3.event.dataTransfer.dropEffect = 'copy';
            return false;
        })
        .on('drop', function(d) {
            d3.event.preventDefault();
            d3.event.stopPropagation();
            args = d3.event.dataTransfer.getData('Text').split(' ');
            if(args[0] === 'brush') {
                brush = args[1]
                porthole.call('setBrushFilter({0}, {1})', plotId, parseInt(brush))
                porthole.call('plots[{0}].getBrush(1).setEnabled(False)', parseInt(brush))
                d3.select(this).text(brush);
                
                //d3.select('#plot' + plotId + ' .slot').text(' ');
            } 
        }); 

        
    plotSlots.append("div")
        .attr('class', 'brush')
        .text("Plot " + plotId + " brush")
        .attr('draggable', true)
        .on('dragstart', function(d) {
            d3.event.dataTransfer.effectAllowed = 'copy';
            d3.event.dataTransfer.setData('Text', "brush " + plotId);
        });
}

//----------------------------------------------------------------------
function setFilter(id, index, field) {
    d3.select('#' + id + " #filterName")
        .text(field);
    porthole.call("setFilter({0}, {1})", index, field);
}

//----------------------------------------------------------------------
// List of fields received. Display in the side bar.
function receiveFieldList(fields) {
    // Select the side panel content
    sidebar = d3.select('#FieldsPanel');
    
    sidebar.selectAll('div')
        .data(fields)
        .enter()
            .append('div')
            .attr('class', 'field')
            .text(function(d) { return d.label; })
            .attr('draggable', true)
            .on('dragstart', function(d) {
                d3.event.dataTransfer.effectAllowed = 'copy';
                d3.event.dataTransfer.setData('Text', "field " + d.label);
            });
            
}
//----------------------------------------------------------------------
// Periodically called to request a new plot image from the server
var IMAGE_REQ_INTERVAL = 100; // in Millis 

//----------------------------------------------------------------------
// Plot image received: display it
function receiveImage(plotId, w, h, data, info) {
    window.setTimeout(function() {
        rect = d3.select("#plot" + plotId).node().getBoundingClientRect();
        plotwidth = rect.width - 80;
        plotheight = rect.height - 180;
        porthole.call("requestImage({0}, {1}, {2}, %client_id%)", plotId, plotwidth, plotheight)
    }, IMAGE_REQ_INTERVAL);
    
    // If width or height = 0 we are receiving an empty refresh (because the pixels)
    // did not change.
    if(w == 0) return;
    
    // Plot position
    px = 20;
    py = 20;
    
    // Padding between axes and actual plot area
    padding = 40;
    
    
    // Select the content div from the main layout panel
    plot = d3.select("#plot" + plotId + " #contentDiv #plotContent");
    if(plot.empty()) {
        plot = d3.select("#plot" + plotId + " #contentDiv");       
        plot = plot.append("svg");
        plot.attr("width", w + padding + px)
            .attr("height", h + padding + py)
            .attr("id", "plotContent");
    } else {
        // Clear the plot area.
        plot.selectAll("*").remove();
        plot.attr("width", w + padding + px)
            .attr("height", h + padding + py)
    }
    
    // Draw plot
    plot.append("svg:image")
        .attr("xlink:href", "data:image/png;base64," + data)
        .attr("x", padding + px)
        .attr("y", padding + py)
        .attr("width", w)
        .attr("height", h)
        
    // Create axis scales             
    xs = d3.scale.linear()
        .domain([info.xmin, info.xmax])
        .range([0, w]);
    ys = d3.scale.linear()
        .domain([info.ymin, info.ymax])
        .range([0, h]);
                        
    xa = d3.svg.axis().scale(xs).orient('top').ticks(10);
    ya = d3.svg.axis().scale(ys).orient('left').ticks(10);
    
    // Draw x axis
    plot.append('g')
        .attr('class', 'axis')
        .attr('transform', 'translate(' + (px + padding) +',' + (py + padding) + ')')
        .call(xa)
        
        
    plot.append('text')
        .text(info.xlabel)
        .attr('class', 'label')
        .attr('transform', 'translate(' + (px + padding + w / 2) +',' + py + ')');
        
    // Draw y axis    
    plot.append('g')
        .attr('class', 'axis')
        .attr('transform', 'translate('+ (px + padding) +',' + ( py + padding) + ')')
        .text(info.ylabel)
        .call(ya)     
    
    plot.append('text')
        .text(info.ylabel)
        .attr('class', 'label')
        .attr('transform', 'translate(' + px +',' + (py + padding + h/2) + ') rotate(-90)');
        
    // Setup brush
    var brush = d3.svg.brush()
        .x(d3.scale.linear().domain([info.xmin, info.xmax]).range([px + padding, w + px + padding]))
        .y(d3.scale.linear().domain([info.ymin, info.ymax]).range([py + padding, h + py + padding]))
        .on("brush", brushed);
        //.on("brushend", brushended);
    skipFirst = true    
    plot.append("g")
        .attr("class", "brush")
        .call(brush)
        .call(brush.event);
        
    function brushed() {
      if(skipFirst) {
          skipFirst = false;
      } else {
          var extent = brush.extent();
          porthole.call("setSelection({0}, {1}, {2}, {3}, {4})", 
            plotId, 
            extent[0][0], extent[1][0],
            extent[0][1], extent[1][1])
            console.log("brush " + extent[0][0] + " " + extent[0][1])
      }
    }

}
    
  
