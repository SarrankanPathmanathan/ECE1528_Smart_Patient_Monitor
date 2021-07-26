import * as functions from "firebase-functions";

// // Start writing Firebase Functions
// // https://firebase.google.com/docs/functions/typescript
//
// export const helloWorld = functions.https.onRequest((request, response) => {
//   functions.logger.info("Hello logs!", {structuredData: true});
//   response.send("Hello from Firebase!");
// });

export const onDataCreate = functions.database
    .ref("root/Test_Results/{DataID}")
    .onCreate((snapshot, context) => {
      const DataID = context.params.DataID;
      const sensorData = snapshot.val();
      const statusUpdateConcern = "Concern";
      const statusUpdateHealthy = "Healthy";
      if (sensorData.bpm > 100 || sensorData.bpm < 40) {
        console.log(`Status Value Updated for ${DataID}`);
        return snapshot.ref.update({status: statusUpdateConcern});
      } else if (sensorData.temperature > 28 || sensorData.temperature < 20) {
        console.log(`Status Value Updated for ${DataID}`);
        return snapshot.ref.update({status: statusUpdateConcern});
      } else if (sensorData.reactionTime > 3000) {
        console.log(`Status Value Updated for ${DataID}`);
        return snapshot.ref.update({status: statusUpdateConcern});
      } else {
        console.log("All parameters good");
        return snapshot.ref.update({status: statusUpdateHealthy});
      }
    });

export const onDataUpdate = functions.database
    .ref("root/Test_Results/{DataID}")
    .onUpdate((change, context) => {
      const DataID = context.params.DataID;
      const befor = change.before.val();
      const after = change.after.val();
      if (befor.bpm === after.bpm && befor.temperature === after.temperature &&
        befor.reactionTime === after.reactionTime) {
        console.log("Text didn't change");
        return null;
      }
      const statusUpdateConcern = "Concern";
      const statusUpdateHealthy = "Healthy";
      if (after.bpm > 100 || after.bpm < 40) {
        console.log(`Status Value Updated for ${DataID}`);
        return change.after.ref.update({status: statusUpdateConcern});
      } else if (after.temperature > 28 || after.temperature < 20) {
        console.log(`Status Value Updated for ${DataID}`);
        return change.after.ref.update({status: statusUpdateConcern});
      } else if (after.reactionTime > 3000) {
        console.log(`Status Value Updated for ${DataID}`);
        return change.after.ref.update({status: statusUpdateConcern});
      } else {
        console.log("All parameters good");
        return change.after.ref.update({status: statusUpdateHealthy});
      }
    });
