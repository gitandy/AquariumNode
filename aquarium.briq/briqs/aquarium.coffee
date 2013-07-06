exports.info =
  name: 'aquarium'
  description: 'This briq controls the AquariumNode'
  menus: [
    title: 'Aquarium'
    controller: 'AquariumCtrl'
  ]
  connections:
    feeds:
      'rf12.packet': 'event'
    results:
      'ss-aquarium': 'event'

state = require '../server/state'
ss = require 'socketstream'

exports.factory = class
  
  constructor: ->
    state.on 'rf12.packet', packetListener
        
  destroy: ->
    state.off 'rf12.packet', packetListener

packetListener = (packet, ainfo) ->
  if packet.id is 2 and packet.group is 1
    buffer = new Buffer packet.buffer 
    temp_sp = packet.buffer[1] / 5.0
    temp_pv = buffer.readFloatLE 2
    fan_spd = buffer.readUInt16LE 6
    ss.api.publish.all 'ss-aquarium', [temp_sp, temp_pv, fan_spd]
