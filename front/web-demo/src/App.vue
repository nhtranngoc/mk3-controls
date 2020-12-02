<template>
<v-app>
  <v-container>
    <v-layout text-xs-center wrap>
      <v-flex xs12 sm6 offset-sm3>
        <v-card>
          <v-card-text>
            <v-container fluid grid-list-lg>
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
            </v-container>
          </v-card-text>
        </v-card>
      </v-flex>
    </v-layout>
  </v-container>
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
      this.$ajax.post('/api/v1/visor/state', {
        isVisorOpen: this.isVisorOpen
      }).then(data => {
        console.log(data)
      }).catch(error => {
        console.log(error)
      })
    }
  },
  methods: {
    setLed: function () {
      this.$ajax
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
