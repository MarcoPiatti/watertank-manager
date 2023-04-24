<template>
      <div class="water-tank" :style="{ height: tankHeight + 'px', width: tankWidth + 'px' }">
        <div class="water-level" :style="{ height: measure.water_level_pct * 100 + '%' }">
          <div class="water-percentage">{{ (measure.water_level_pct * 100).toFixed(0) }}%</div>
        </div>
      </div>
      <div class="water-liters" :style="{ top: '-40px' }">{{ measure.water_level_lts.toFixed(0) }} L</div>
</template>

<script lang="ts">
import { defineComponent } from 'vue';
import type { PropType } from 'vue';
import axios from 'axios';

export default defineComponent({
  name: 'WaterTank',
  props: {
    tankWidth: {
      type: Number as PropType<number>,
      default: 200
    },
    tankHeight: {
      type: Number as PropType<number>,
      default: 300
    }
  },
  data() {
    return {
      tankInfo: {
        sensor_pos_cm: 0,
        capacity_lts: 0,
        capacity_cm: 0,
        low_pct: 0,
        high_pct: 0
      },
      measure: {
        water_level_pct: 0.7,
        water_level_lts: 700,
        water_level_cm: 0
      }
    }
  },
  
  mounted () {
    axios.get('/tank').then(response => {this.tankInfo = response.data;});   
    axios.get('/measure').then(response => (this.measure = response.data));
  }
});
</script>
  
<style scoped>

.water-tank-info {

}

.water-tank {
  position: relative;
  background-color: #e6e6e6;
  border: 5px solid #b3b3b3;
  border-radius: 20px;
  margin: 0 auto;
  overflow: hidden;
}
  
.water-level {
  position: absolute;
  bottom: 0;
  left: 0;
  right: 0;
  background-color: #2d8dd7;
  opacity: 0.8;
  transition: height 1s;
}

.water-percentage {
  position: absolute;
  font-size: 20px;
  font-weight: bold;
  text-align: center;
  width: 100%;
  color: whitesmoke;
}

.water-liters {
  position: absolute;
  font-size: 20px;
  font-weight: bold;
  text-align: center;
  width: 100%;
  color: dimgray;
}
  
</style>
  