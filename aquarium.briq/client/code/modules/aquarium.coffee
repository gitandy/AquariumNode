module.exports = (ng) ->

  ng.controller 'AquariumCtrl', [
    '$scope',
    ($scope) ->
      $scope.$on 'ss-aquarium', (event, value) ->
        $scope.temp_sp = value[0] / 5.0
        $scope.temp_pv = value[1] / 5.0
        $scope.fan_spd = value[2]
        
        if $scope.temp_pv > $scope.temp_sp+2
          $scope.txt_color = 'red'
        else if $scope.temp_pv > $scope.temp_sp+1
          $scope.txt_color = 'darkorange'
        else if $scope.temp_pv > $scope.temp_sp+0.5
          $scope.txt_color = 'gold'
        else
          $scope.txt_color = 'green'  	
  ]
