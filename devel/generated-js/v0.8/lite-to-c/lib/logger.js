//Logger Utility
//==============


//Dependencies:
//-------------

    //import color, ControlledError, GeneralOptions
    var color = require('./color.js');
    var ControlledError = require('./ControlledError.js');
    var GeneralOptions = require('./GeneralOptions.js');

    //global import fs
    var fs = require('fs');
    //import mkPath 
    var mkPath = require('./mkPath.js');

//## Main namespace

//### Namespace logger
    var logger={};

//#### properties 

        //options: GeneralOptions
        //errorCount = 0
        //warningCount = 0

//if storeMessages, messages are pushed at messages[] instead of console.

        //storeMessages: boolean
        //messages: string Array = []
         logger.errorCount=0;
         logger.warningCount=0;
         logger.messages=[];

//Implementation
//---------------

//#### method init(options:GeneralOptions)
     logger.init = function(options){

        //logger.options = options
        logger.options = options;

        //logger.errorCount = 0
        logger.errorCount = 0;
        //logger.warningCount = 0
        logger.warningCount = 0;

        //logger.storeMessages=false
        logger.storeMessages = false;
        //logger.messages=[]
        logger.messages = [];
     };


//#### method debug
     logger.debug = function(){

        //if logger.options.debugEnabled
        if (logger.options.debugEnabled) {
            //var args = arguments.toArray()
            var args = Array.prototype.slice.call(arguments);
            //console.error.apply undefined,args
            console.error.apply(undefined, args);
        };
     };

//#### method debugGroup
     logger.debugGroup = function(){

        //if logger.options.debugEnabled
        if (logger.options.debugEnabled) {
            //console.error.apply undefined,arguments
            console.error.apply(undefined, Array.prototype.slice.call(arguments));
            //console.group.apply undefined,arguments
            console.group.apply(undefined, Array.prototype.slice.call(arguments));
        };
     };

//#### method debugGroupEnd
     logger.debugGroupEnd = function(){

        //if logger.options.debugEnabled
        if (logger.options.debugEnabled) {
            //console.groupEnd
            console.groupEnd();
        };
     };

//#### method error
     logger.error = function(){

//increment error count 

        //logger.errorCount++
        logger.errorCount++;
        //var args = arguments.toArray()
        var args = Array.prototype.slice.call(arguments);

//add "ERROR:", send to debug logger

        //args.unshift('ERROR:')
        args.unshift('ERROR:');
        //logger.debug.apply undefined,args
        logger.debug.apply(undefined, args);

//if messages should be stored...

        //if logger.storeMessages
        if (logger.storeMessages) {
            //logger.messages.push args.join(" ")
            logger.messages.push(args.join(" "));
        }
        else {

//else, add red color, send to stderr

        //else
            //args.unshift(color.red)
            args.unshift(color.red);
            //args.push(color.normal)
            args.push(color.normal);
            //console.error.apply undefined,args
            console.error.apply(undefined, args);
        };
     };


//#### method warning
     logger.warning = function(){

        //logger.warningCount++
        logger.warningCount++;
        //var args = arguments.toArray()
        var args = Array.prototype.slice.call(arguments);

        //args.unshift('WARNING:')
        args.unshift('WARNING:');
        //logger.debug.apply(undefined,args)
        logger.debug.apply(undefined, args);

        //if logger.options.warningLevel > 0
        if (logger.options.warningLevel > 0) {

//if messages should be stored...

            //if logger.storeMessages
            if (logger.storeMessages) {
                //logger.messages.push args.join(" ")
                logger.messages.push(args.join(" "));
            }
            else {

//else, add yellow color, send to stderr

            //else
                //args.unshift(color.yellow);
                args.unshift(color.yellow);
                //args.push(color.normal);
                args.push(color.normal);
                //console.error.apply(undefined,args);
                console.error.apply(undefined, args);
            };
        };
     };

//#### method msg
     logger.msg = function(){

        //var args = arguments.toArray()
        var args = Array.prototype.slice.call(arguments);

        //logger.debug.apply(undefined,args)
        logger.debug.apply(undefined, args);
        //if logger.options.verboseLevel >= 1
        if (logger.options.verboseLevel >= 1) {

//if messages should be stored...

            //if logger.storeMessages
            if (logger.storeMessages) {
                //logger.messages.push args.join(" ")
                logger.messages.push(args.join(" "));
            }
            else {

//else, send to console

            //else 
                //console.log.apply(undefined,args)
                console.log.apply(undefined, args);
            };
        };
     };


//#### method info
     logger.info = function(){

        //var args = arguments.toArray()
        var args = Array.prototype.slice.call(arguments);
        //if logger.options.verboseLevel >= 2
        if (logger.options.verboseLevel >= 2) {
            //logger.msg.apply(undefined,args)
            logger.msg.apply(undefined, args);
        };
     };

//#### method extra
     logger.extra = function(){

        //var args = arguments.toArray()
        var args = Array.prototype.slice.call(arguments);
        //if logger.options.verboseLevel >= 3
        if (logger.options.verboseLevel >= 3) {
            //logger.msg.apply(undefined,args)
            logger.msg.apply(undefined, args);
        };
     };


//#### method getMessages
     logger.getMessages = function(){
//get & clear

        //var result = logger.messages
        var result = logger.messages;
        //logger.messages =[]
        logger.messages = [];
        //return result
        return result;
     };


//#### method throwControlled(msg)
     logger.throwControlled = function(msg){
//Throws Error, but with a "controlled" flag set, 
//to differentiate from unexpected compiler errors

        //logger.debug "Controlled ERROR:", msg
        logger.debug("Controlled ERROR:", msg);
        //throw new ControlledError(msg)
        throw new ControlledError(msg);
     };
// --------------------
// Module code
// --------------------
// end of module



module.exports=logger;
