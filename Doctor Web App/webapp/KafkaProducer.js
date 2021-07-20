// import the `Kafka` instance from the kafkajs library
const { Kafka } = require("kafkajs")

// the client ID lets kafka know who's producing the messages
const clientId = "my-app"
// we can define the list of brokers in the cluster
const brokers = ["172.17.28.231:9092"]
// this is the topic to which we want to write messages
const topic = "Alerts"

// initialize a new kafka client and initialize a producer from it
const kafka = new Kafka({ clientId, brokers })
const producer = kafka.producer()

// we define an async function that writes a new message each second
const produce = async () => {
    await producer.connect()
    let i = 0
    try {
        // send a message to the configured topic with
        // the key and value formed from the current value of `i`
        await producer.send({
            topic:topic,
            messages: [
                {
                    id:"hi",
                    key:"patientAlerted",
                    value:"true"
                },
            ],
        })

        // if the message is written successfully, log it and increment `i`
        console.log("writes: ", i)
        i++
    } catch (err) {
        console.error("could not write message " + err)
    }
}

module.exports = produce