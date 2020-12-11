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
                <v-slider v-model="led" max="255" step="25" ticks @change="setLed" :disabled="!isSet"></v-slider>
              </v-col>
              <v-col cols="4">
                <v-subheader>Visor Open</v-subheader>
              </v-col>
              <v-col cols="7">
                <v-switch v-model="isVisorOpen" :disabled="!isSet"></v-switch>
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
      led: 127, 
      isVisorOpen: false,
      connection: null, 
      isSet: false,
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
      this.isSet = true; // first setup
    },
    getVisor: function(v) {
      this.isVisorOpen = v;
      this.isSet = true; 
    },
    setLed: function() {
      console.log("LED value: " + this.led);
      this.connection.send("sl" + parseInt(this.led)); // Server takes an 8-bit unsigned value for led brightness
      this.isSet = false;
    },
    setVisor: function(v) {
      // Only fire when the watch value is different to prevent loop
      if (this.isVisorOpen === v) {
        return;
      }
      console.log("Visor state: " + v);
      this.connection.send("sv" + v);
      this.isSet = false;
    },
    handleMessage: function(msg) {
      console.log("Receiving message: " + msg);
      if (msg === "ok") {
        console.log("Success");
        this.isSet = true;
      } else {
        const value = parseInt(msg.substring(1));
        if (msg.startsWith('l')) {
          console.log("Get LED value: " + value);
          this.getLed(parseInt(value));
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
