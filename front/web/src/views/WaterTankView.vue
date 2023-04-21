<script setup lang="ts">
import WaterTank from '../components/WaterTank.vue'
</script>

<template>
  <main>
    <WaterTank :capacity=100 :water-level=50 />
  </main>
</template>
