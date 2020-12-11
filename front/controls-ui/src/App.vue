<template>
<v-app id="app">
  <v-container fluid>
    <v-row>
      <v-col>
        <v-img src="https://raw.githubusercontent.com/nhtranngoc/mk3-controls/master/ironman.png" />
      </v-col>
      <v-col>
        <v-container fill-height>
          <transition name="fade">
            <v-row align-items="center" justify="start">
              <v-col cols="12" class="header">
                <h1>MKIII Controls</h1>
              </v-col>
              <v-col cols="4">
                <v-subheader>LED Brightness</v-subheader>
              </v-col>
              <v-col cols="7">
                <v-slider v-model="led" max="100" step="10" ticks @change="setLed"></v-slider>
              </v-col>
              <v-col cols="4">
                <v-subheader>Visor Open</v-subheader>
              </v-col>
              <v-col cols="7">
                <v-switch v-model="isVisorOpen"></v-switch>
              </v-col>
              <v-spacer/>
            </v-row>
          </transition>
        </v-container>
      </v-col>
    </v-row>
  </v-container>
</v-app>
</template>

<script>
export default {
  name: 'App',
  data () {
    return { 
      led: 50, 
      isVisorOpen: false,
      connection: null, 
    }
  },
  created() {
    console.log("Starting connection to WebSocket Server")
    this.connection = new WebSocket('wss://' + window.location.hostname + '/ws')

    this.connection.onmessage = (event) => {
      console.log(event);
      this.handleMessage(event.data)
    }

    this.connection.onopen = (event) =>  {
      console.log("On open register...");
      console.log(event);
      this.connection.send("g");
      console.log("Successfully connected to the echo websocket server...")
    };
    
  },
  watch: {
    isVisorOpen (v) {
      let valueInInt = v ? 1 : 0;
      this.setVisor(valueInInt);
    }
  },
  methods: {
    getLed: function(v) {
      this.led = v;
    },
    getVisor: function(v) {
      this.isVisorOpen = v;
    },
    setLed: function() {
      console.log("LED value: " + this.led);
      this.connection.send("l" + parseInt(this.led/100*255)); // Server takes an 8-bit unsigned value for led brightness
    },
    setVisor: function(v) {
      console.log("Visor state: " + v);
      this.connection.send("v" + v);
    },
    handleMessage: function(msg) {
      console.log("Receiving message: " + msg);
      if (msg === "ok") {
        console.log("Success");
      } else {
        const value = parseInt(msg.substring(1));
        if (msg.startsWith('l')) {
          console.log("Get LED value: " + value);
          this.getLed(parseInt(value/225*100));
        }
        if (msg.startsWith('v')) {
          console.log("Get Visor value: " + value);
          this.getVisor(value);
        }
      }

    }
  }
}
</script>


<style>
html {
    height: 100vh;
}

#app {
  font-family: Avenir, Helvetica, Arial, sans-serif;
  -webkit-font-smoothing: antialiased;
  -moz-osx-font-smoothing: grayscale;
  color: #2c3e50;
  background-color: #FBFAF3;
}

.header {
  margin-top: 2em;
}

h1 {
  font-family: Avenir;
  color: gray;
  text-align: center;
}

.fade-enter-active, .fade-leave-active {
  transition-property: opacity;
  transition-duration: .35s;
}
.fade-enter-active {
  transition-delay: .35s;
}
.fade-enter, .fade-leave-active {
  opacity: 0
}

</style>
