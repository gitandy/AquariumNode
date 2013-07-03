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
    temp_sp = packet.buffer[1]
    temp_pv = packet.buffer[2]
    fan_spd = packet.buffer[3]
    ss.api.publish.all 'ss-aquarium', [temp_sp, temp_pv, fan_spd]
