<template>
<v-app>
  <div id="app">
    <v-layout>
      <v-row>
      <v-col cols="8" sm6>
        <img src="./assets/ironman.png" />
      </v-col>
      <v-col cols="4" sm6>
          <v-layout row wrap>
            <v-flex xs3>
              <v-subheader>LED</v-subheader>
            </v-flex>
            <v-flex xs9>
              <v-slider v-model="led" max="100" @change="setLed"></v-slider>
            </v-flex>
            <v-flex xs3>
              <v-subheader>Visor Open</v-subheader>
            </v-flex>
            <v-flex xs9>
              <v-switch v-model="isVisorOpen"></v-switch>
            </v-flex>
          </v-layout>
      </v-col>
      </v-row>
    </v-layout>
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
      this.$http.post('/api/v1/visor/state', {
        isVisorOpen: v
      }).then(data => {
        console.log(data)
      }).catch(error => {
        console.log(error)
      })
    }
  },
  methods: {
    setLed: function () {
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
}
</style>
