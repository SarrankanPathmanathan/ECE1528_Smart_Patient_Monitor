// index.js

/**
 * Required External Modules
 */

const express = require("express");
const path = require("path");
const expressSession = require("express-session");
const passport = require("passport");
const Auth0Strategy = require("passport-auth0");
var Plot = require("nodeplotlib");
var firebase = require("firebase");
const produce = require('C:\\Users\\SarrankanPathmanatha\\Documents\\ECE1528\\webapp\\KafkaProducer');

firebase.initializeApp({
    databaseURL: "https://ece1528-43d8d-default-rtdb.firebaseio.com/"
});


require("dotenv").config();


const authRouter = require("./auth");

/**
 * App Variables
 */

const app = express();
const http = require('http').Server(app);
const port = process.env.PORT || "3000";

const io = require('socket.io')(http);

/**
 * Session Configuration
 */

const session = {
    secret: "LoxodontaElephasMammuthusPalaeoloxodonPrimelephas",
    cookie: {},
    resave: false,
    saveUninitialized: false
};

if (app.get("env") === "production") {
    // Serve secure cookies, requires HTTPS
    session.cookie.secure = true;
}

/**
 * Passport Configuration
 */
const strategy = new Auth0Strategy(
    {
        domain: process.env.AUTH0_DOMAIN,
        clientID: process.env.AUTH0_CLIENT_ID,
        clientSecret: process.env.AUTH0_CLIENT_SECRET,
        callbackURL: process.env.AUTH0_CALLBACK_URL || "http://localhost:3000/callback"
    },
    function(accessToken, refreshToken, extraParams, profile, done) {
        /**
         * Access tokens are used to authorize users to an API
         * (resource server)
         * accessToken is the token to call the Auth0 API
         * or a secured third-party API
         * extraParams.id_token has the JSON Web Token
         * profile has all the information from the user
         */
        return done(null, profile);
    }
);


/**
 *  App Configuration
 */

app.set("views", path.join(__dirname, "views"));
app.set("view engine", "pug");
app.use(express.static(path.join(__dirname, "public")));

app.use(expressSession(session));

passport.use(strategy);
app.use(passport.initialize());
app.use(passport.session());

passport.serializeUser((user, done) => {
    done(null, user);
});

passport.deserializeUser((user, done) => {
    done(null, user);
});

// Creating custom middleware with Express
app.use((req, res, next) => {
    res.locals.isAuthenticated = req.isAuthenticated();
    next();
});

// Router mounting
app.use("/", authRouter);

/**
 * Routes Definitions
 */

var patient1Data = [];
var patient2Data = [];
var currentPatient;
var patientAnalysis;

var dbRef = firebase.database().ref("root/Test_Results");
dbRef.limitToLast(1).on("child_added", (snap) => {
    lastTest = snap.val().id;
});

// firebase db reference
var ref = firebase.database().ref("root/Test_Results");


console.log(patient1Data.length);

app.get("/", (req, res) => {
    res.render("index", { title: "Home" });
});



const secured = (req, res, next) => {
    if (req.user) {
        return next();
    }
    req.session.returnTo = req.originalUrl;
    res.redirect("/login");
};

app.get("/patient/1", secured, (req, res, next) => {
    const { _raw, _json, ...userProfile } = req.user;
    userProfile.name = "Patient 1"
    patient1Data = [];
    getTestResults();
    currentPatient = "Patient 1";
    res.render("user", {
        title: "Profile",
        userProfile: userProfile,
        testData: patient1Data
    });
});

app.get("/patient/2", secured, (req, res, next) => {
    const { _raw, _json, ...userProfile } = req.user;
    userProfile.name = "Patient 2"
    patient2Data = [];
    getTestResults();
    currentPatient = "Patient 2";
    res.render("user", {
        title: "Profile",
        userProfile: userProfile,
        testData: patient2Data
    });
});

app.get("/live", secured, (req, res, next) => {
    const { _raw, _json, ...userProfile } = req.user;
    res.render("live", {
        title: "Live Status",
        userProfile: userProfile,
        testData: patient2Data
    });
});

app.get("/alert", secured, (req, res, next) => {
    console.log("Alert");
    produce().catch((err) => {
        console.error("error in producer: ", err)
    })
});

app.get("/analyze", secured, (req, res, next) => {
    const { _raw, _json, ...userProfile } = req.user;
    if(currentPatient == "Patient 1"){
        userProfile.name = "Patient 1"
        patient1Data = [];
        getTestResults();
        patientData = patient1Data;
        patientAnalysis = performAnalysis("1",patient1Data);
    }else {
        userProfile.name = "Patient 2"
        patient2Data = [];
        getTestResults();
        patientData = patient2Data;
        patientAnalysis = performAnalysis("2",patient2Data);
    }
    if(patientAnalysis.length == 0){
        patientAnalysis = "";
    }
    res.render("analysis", {
        title: "Analysis",
        userProfile: userProfile,
        testData: patientData,
        analysis: patientAnalysis,
    });
});

app.get("/charts", secured, (req, res, next) => {
    const { _raw, _json, ...userProfile } = req.user;
    const heartData = [{x:patientAnalysis.testTime, y:patientAnalysis.testBPM,type:'line'}];
    const heartLayout ={name:"Hello"};
    const reactionData = [{x:patientAnalysis.testTime, y:patientAnalysis.testReaction,type:'line'}];
    const temperatureData = [{x:patientAnalysis.testTime, y:patientAnalysis.testTemp,type:'bar'}];
    Plot.stack(heartData, heartLayout);
    Plot.stack(reactionData);
    Plot.stack(temperatureData);
    Plot.plot();
});

function performAnalysis(patientId, patientData){
    var analysis = {Incomplete:[],AbnormalBPM:[],AbnormalTemperature:[],AbnormalReactionTime:[], testIds:[], testTime:[], testBPM:[],testTemp:[],testReaction:[]}
    analysis.id = patientId;
    patientData.forEach(function(test){
        if (test.status == "Incomplete"){
            analysis["Incomplete"].push(test);
        }
        if (test.bpm > 90 || test.bpm < 40) {
            analysis["AbnormalBPM"].push(test);
        }
        if (test.temperature < 20){
            analysis["AbnormalTemperature"].push(test);
        }
        if (test.temperature < 20){
            analysis["AbnormalTemperature"].push(test);
        }
        if (test.reactionTime < 100 || test.reactionTime > 3000){
            analysis["AbnormalReactionTime"].push(test);
        }
        analysis.testIds.push(test.id);
        analysis.testTime.push(test.timestamp);
        analysis.testTemp.push(test.temperature);
        analysis.testBPM.push(test.bpm);
        analysis.testReaction.push(test.reactionTime);
    })
    return analysis
}

function getTestResults(){
    dbRef.orderByChild("id").on("child_added", snap => {
        testData = snap.val();
        if(testData.patientId == "Patient_1"){
            patient1Data.push(testData);
        }else{
            patient2Data.push(testData);
        }
    });
}


/**
 * Routes Definitions
 */

app.get("/", (req, res) => {
    res.render("index", { title: "Home" });
});

/**
 * Server Activation
 */

http.listen(3000, () => {
    console.log("Server running on port 3000");
});

