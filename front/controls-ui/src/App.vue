<template>
<v-app>
  <div id="app">
    <v-container>
      <v-row>
        <v-col cols="6" sm6>
            <v-container fill-height>
              <transition name="fade">
              <v-row align-items="center" justify="start">
                <v-col cols="12">
                  <h1>MK3 Controls</h1>
                </v-col>
                <v-col cols="4">
                  <v-subheader>LED Brightness</v-subheader>
                </v-col>
                <v-col cols="8">
                  <v-slider v-model="led" max="100" step="10" ticks @change="setLed"></v-slider>
                </v-col>
                <v-col cols="4">
                  <v-subheader>Visor Open</v-subheader>
                </v-col>
                <v-col cols="8">
                  <v-switch v-model="isVisorOpen"></v-switch>
                </v-col>
              </v-row>
              </transition>
            </v-container>
        </v-col>
        <v-col cols="6" sm6>
          <img src="./assets/ironman.png" />
        </v-col>
      </v-row>
    </v-container>
  </div>
</v-app>
</template>

<script>
export default {
  name: 'App',
  data () {
    return { led: 50, isVisorOpen: false }
  },
  watch: {
    isVisorOpen (v) {
      let valueInInt = v ? 1 : 0;
      console.log("Visor state: " + valueInInt);
      this.$http.post('/api/v1/visor/state', {
        isVisorOpen: valueInInt
      }).then(data => {
        console.log(data)
      }).catch(error => {
        console.log(error)
      })
    }
  },
  methods: {
    setLed: function () {
      console.log("LED value: " + this.led);
      this.$http
        .post('/api/v1/light/brightness', {
          led: this.led
        })
        .then(data => {
          console.log(data)
        })
        .catch(error => {
          console.log(error)
        })
    }
  }
}
</script>


<style>
#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  text-align: center;
  color: #2c3e50;
  margin-top: 60px;
  background-color: #FBFAF3;
}

h1 {
  font-family: Avenir;
  color: gray;
}

.fade-enter-active, .fade-leave-active {
  transition-property: opacity;
  transition-duration: .35s;
}

.fade-enter-active {
  transition-delay: .35s;
}

.fade-enter, .fade-leave-active {
  opacity: 0;
}

</style>
