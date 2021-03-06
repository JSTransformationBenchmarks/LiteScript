Name Validation 
===============

This module contains helper functions to manage variable, 
function and object property name declaration.

This module purpose is to make the compiler catch 
mistyped variable and property names at compile time
(instead of YOU spending hours to debug a subtle bug at run time)

In order to do name validation we need to construct the scope tree, 
and also register all valid members of all "types" (objects).

----------
##Dependencies:

This module extends Grammar classes, adding 'declare', 'evaluateAssignments', etc.
methods to validate var & property names.

    import 
        ASTBase, Grammar
        Names, Environment
        logger, UniqueID

    shim import LiteCore,Map
    
---------
Module vars:

    var project

    var globalScope: Names.Declaration
    var globalObjectProto: Names.Declaration

    var nameAffinity: Names.Declaration


##Members & Scope

A Names.Declaration have a `.members=Map string to NamedDeclaration` property
`.members={}` is a map to other `Names.Declaration`s which are valid members of this name.

A 'scope' is a Names.Declaration whose members are the declared vars in the scope.

For Example: 'console' is stored at 'Global Scope' and has '.log' and '.error' as members

Project
|_ 
   scope = {
     name: 'global scope'
     members: {
        console: { 
          name:'console'
          type: Object
          members: 
              log: 
                name:'log'
                type: Function
              error:
                name: 'error'
                type: Function
          }
     }


'Scopes' are created only for certain AST nodes, such as:
Module, FunctionDeclaration, ForStatement, Catch/Exception, etc.

Variables in the scope
----------------------
Referenced vars must be in the scope . You are required to explicitly declare your variables
so you're **unable** to create a global variable by mistipying a name in an assignment. 
The compiler will catch such a misstype as "undeclared variable". 

Object properties access
------------------------
Object properties access are another source of subtle bugs in any medium to large javascript project.
The problem is a mistyped property name results in the property not being found 
in the object nor the prototype chain, and javascript in this case just returns "undefined" 
and goes on. This causes hard to find subtle bugs.

Example: The following javascript code, **will probably need debugging.**

  options = options || {};
  if (options.importantCodeDefaultTrue===undefined) options.importantCodeDefaultTrue=true;
  if (options.anotherOptionDefaultZero===undefined) options.anotherOptionDefaultZero=0;

  initFunction(options);
  prepareDom(options);
  if (options.importantCodesDefaultTrue) { moreInit(); subtleDomChanges(); }
  
The same LiteScript code, but the mistake **will be caught by the compiler**
The compiler will emit an error during compilation, -no debugging required-.

  options = options or {}
  if options.importantCodeDefaultTrue is undefined, options.importantCodeDefaultTrue=true
  if options.anotherOptionDefaultZero is undefined, options.anotherOptionDefaultZero=0;

  initFunction options
  prepareDom options
  if options.importantCodesDefaultTrue then moreInit(); subtleDomChanges()

In order to completely check property names, a full type system is neeeded.

LiteScript, based in js, *is not typed*, but you can add "type annotations"
to your variable declaration, in order to declare the list of valid members 
to check at compile time.

The compiler will guess var types from creation, assignment
and by name affinity. If type cannot be guessed you can also explicitily use a 
`declare on myObj prop1,prop2` statement to dismiss the 'UNDECLARED PROPERTY' warnings.

Example:
/*

  class ClassA

    properties 
      classAProp1, classAProp2
    
    method methodA
      this.classAProp1 = 11
      this.classAProp2 = 12

  class ClassB
    
    properties 
      classBProp1, classBProp2

    method methodB
      this.classBProp1 = 21

  var instanceB = new ClassB // implicit type

  instanceB.classBprop1 = 5 // <-- this **will be caught** as "case mismatch" since classBprop1 is defined as classBProp1

  var bObj = instanceB // simple assignment, implicit type

  bObj.classAProp1 = 5 // <-- this **will be caught** as: object 'bObj' has no property 'classAProp1'

  var xObj = callToFn() // unknown type
  
  xObj.classBProp1 = 5 // <-- this trigger a "classBProp1 IS NOT A DECLARED PROPERTY OF xObj"

  declare on xObj  // <-- this fixes it
    classBProp1

  xObj.classBProp1 = 5 // <-- this is OK now

  var xObj:ClassB = callToFn() // type annotation, this also fixes it
  
  bObj.classBProp1 = 5 // <-- this is ok

*/


### export function execute()

We start this validation process after the entire multi-module AST tree has been parsed.

Start running passes on the AST

#### Pass 1.0 Declarations 
Walk the tree, and call function 'declare' on every node having it. 
'declare' will create scopes, and vars in the scope. 
May inform 'DUPLICATES' and 'CASE MISMATCH' errors.

        logger.info "- Process Declarations"
        walkAllNodesCalling 'declare'

/*
#### Pass 1.1 Declare By Assignment
Walk the tree, and check assignments looking for: 'module.exports.x=x' and 'x.prototype.y = '.
Treat them as declarations.

        logger.info "- Declare By Assignment (support .js syntax, .exports.x=..., .prototype.x=...)"
        walkAllNodesCalling 'declareByAssignment'
*/

#### Pass 1.2 connectImportRequire

        logger.info "- Connect Imported"

validate public exports.
set module.exports with default export object if set

        for each moduleNode:Grammar.Module in map project.moduleCache

            moduleNode.confirmExports 

            //for interfaces, connect alias to vars & props
            //this is to support jQuery.fn = prototype and "append to namespace jQuery.fn" as alias to "append to namespace jQuery.prototype"
            if moduleNode.fileInfo.isInterface
                moduleNode.callOnSubTree "connectAlias" //only vardecls have this method

handle: `import x` and `global declare x`
Make var x point to imported module 'x' exports 

        for each moduleNode:Grammar.Module in map project.moduleCache

          for each node in moduleNode.requireCallNodes

            if node.importedModule

              var parent: ASTBase
              var referenceNameDecl: Names.Declaration //var where to import exported module members

              declare valid parent.nameDecl

1st, more common: if node is Grammar.ImportStatementItem

              if node instance of Grammar.ImportStatementItem
                  declare node:Grammar.ImportStatementItem
                  referenceNameDecl = node.nameDecl

if we process a 'global declare' command (interface) 
all exported should go to the global scope.

If the imported module exports a class, e.g.: "export default class OptionsParser",
'importedModule.exports' points to the class 'prototype'. 
            
                  if node.getParent(Grammar.DeclareStatement) isnt undefined //is a "global declare"

                        if not node.importedModule.movedToGlobal //already processed
                            var moveWhat = node.importedModule.exports
                            #if the module has a export-only class, move to global with class name
                            if moveWhat.findOwnMember('prototype') into var protoExportNameDecl 
                                //if it has a 'prototype'
                                //replace 'prototype' (on module.exports) with the class name, and add as the class
                                protoExportNameDecl.name = protoExportNameDecl.parent.name
                                project.rootModule.addToScope protoExportNameDecl
                          
                            else
                                // a "declare global x", but "x.lite.md" do not export a class
                                // move all exported (namespace members) to global scope
                                for each nameDecl in map moveWhat.members
                                    project.rootModule.addToScope nameDecl

                            //mark as processed
                            node.importedModule.movedToGlobal = true

                        //we moved all to the global scope, e.g.:"declare global jQuery" do not assign to referenceNameDecl
                        referenceNameDecl = undefined

                  //else 
                      //commented: is valid that some modules to export nothing
                      // e.g.: module "shims" and a main module 
                      //if node.importedModule.exports.members.size is 0
                      //    node.warn "nothing exported in #{node.importedModule.fileInfo.filename}"

/*

else is a "require" call (VariableRef). 
Get parent node.

              else
                  parent = node.parent
                  if parent instance of Grammar.Operand 
                     parent = node.parent.parent.parent # varRef->operand->Expression->Expression Parent

get referece where import module is being assigned to

                  if parent instance of Grammar.AssignmentStatement 
                      var opt = new Names.NameDeclOptions
                      opt.informError = true
                      declare valid parent.lvalue.tryGetReference
                      referenceNameDecl = parent.lvalue.tryGetReference(opt) 
                  
                  else if parent instance of Grammar.VariableDecl
                      referenceNameDecl = parent.nameDecl

              end if
*/

After determining referenceNameDecl where imported items go,
make referenceNameDecl point to importedModule.exports

              if referenceNameDecl
                  referenceNameDecl.makePointTo node.importedModule.exports
                  // if it has a 'prototype' => it's a Function-Class
                  // else we assume all exported from module is a namespace
                  //referenceNameDecl.isNamespace = no referenceNameDecl.findOwnMember('prototype') 


#### Pass 1.3 Process "Append To" Declarations
Since 'append to [class|object] x.y.z' statement can add to any object, we delay 
"Append To" declaration to this point, where 'x.y.z' can be analyzed and a reference obtained.
Walk the tree, and check "Append To" Methods & Properties Declarations

        logger.info "- Processing Append-To, extends"
        walkAllNodesCalling 'processAppendToExtends'


#### Pass 2.0 Apply Name Affinity

        logger.info "- Apply Name Affinity"

        #first, try to assign type by "name affinity" 
        #(only applies when type is not specified)
        for each nameDecl in Names.allNameDeclarations 
            nameDecl.assignTypebyNameAffinity()

#### Pass 2.1 Convert Types
for each Names.Declaration try to find the declared 'type' (string) in the scope. 
Repeat until no conversions can be made.

        logger.info "- Converting Types"

        #now try de-referencing types
        var pass=0, sumConverted=0, sumFailures=0, lastSumFailures=0
        #repeat until all converted
        do

            lastSumFailures = sumFailures
            sumFailures = 0
            sumConverted = 0
            
            #process all, sum conversion failures
            for each nameDecl in Names.allNameDeclarations 
                var result = nameDecl.processConvertTypes()
                sumFailures += result.failures
                sumConverted += result.converted
            end for

            pass++
            //logger.debug "  -  Pass #{pass}, converted:#{sumConverted}, failures:#{sumFailures}"

        #loop unitl no progress is made
        loop until sumFailures is lastSumFailures

Inform unconverted types as errors

        if sumFailures #there was failures, inform al errors
            for each nameDecl in Names.allNameDeclarations
                nameDecl.processConvertTypes({informError:true})

#### Pass 3 Evaluate Assignments
Walk the scope tree, and for each assignment, 
IF L-value has no type, try to guess from R-value's result type

        logger.info "- Evaluating Assignments"
        walkAllNodesCalling 'evaluateAssignments'

#### Pass 4 -Final- Validate Property Access
Once we have all vars declared and typed, walk the AST, 
and for each VariableRef validate property access.
May inform 'UNDECLARED PROPERTY'.

        logger.info "- Validating Property Access"
        walkAllNodesCalling 'validatePropertyAccess'

Inform forward declarations not fulfilled, as errors

        for each nameDecl in Names.allNameDeclarations

            if nameDecl.isForward and not nameDecl.isDummy

                nameDecl.warn "forward declared, but never found"
                var container = nameDecl.nodeDeclared.getParent(Grammar.ClassDeclaration)
                if container
                  declare container:Grammar.ClassDeclaration
                  declare valid container.varRef.toString
                  if container.varRef, logger.warning "#{container.positionText()} more info: '#{nameDecl.name}' of '#{container.varRef.toString()}'"

    end function validate

### export function walkAllNodesCalling(methodName:string)

        var methodSymbol = LiteCore.getSymbol(methodName)

        if project.moduleCache.size is 0 
            throw new Error("walkAllNodes: no modules in moduleCache")

For all modules, for each node, if the specific AST node has methodName, call it

        for each moduleNode:Grammar.Module in map project.moduleCache
            moduleNode.callOnSubTree methodSymbol


### export function initialize(aProject)

Initialize module vars

        project = aProject
        
        #clear global Names.Declaration list
        Names.allNameDeclarations = []

initialize NameAffinity

        # project-wide name affinity for classes
        nameAffinity = new Names.Declaration('Name Affinity',{
            normalizeModeKeepFirstCase:true #nameAffinity members are stored: [0].Toupper()+slice(1).toLower()
            nodeClass: Grammar.VariableDecl
            });

        //populateGlobalScope(aProject)

The "scope" of rootNode is the global scope. 

        globalScope = project.rootModule.createScope()

Initialize global scope
a)non-instance values

        var opt = new Names.DeclarationOptions
        opt.nodeClass = Grammar.VariableDecl
        globalScope.addMember 'undefined',opt
        opt.value = null
        globalScope.addMember 'null',opt
        opt.value = true
        globalScope.addMember 'true',opt
        opt.value = false
        globalScope.addMember 'false',opt
        opt.value = NaN
        globalScope.addMember 'NaN',opt
        opt.value = Infinity
        globalScope.addMember 'Infinity',opt

b) pre-create core classes, to allow the interface.md file to declare property types and return values

        AddGlobalClasses 
            'Object', 'Function', 'Array' 
            'String', 'Number', 'Boolean'

In JS the global environment (global|window) is a *Object*, and as such it 
*has* Object.prototype in its prototype chain, which means 
*all properties in Object.prototype are also in the global scope*

Get hold of Object.prototype since we're using it as "parent" (__proto__) of the global scope.

        globalObjectProto = tryGetGlobalPrototype('Object')

Allow use of "__proto__" getter/setter on any object

        globalObjectProto.addMember '__proto__',{nodeClass:Grammar.VariableDecl}
            
note: 'Map' and 'NameValuePair' are declared at GlobalScopeX.interface.md

b) create special types

-"any" default type for vars

        globalScope.addMember 'any',{nodeClass:Grammar.ClassDeclaration} // used for "map string to any" - Dictionaries

-arguments:any*

"arguments:any*" - arguments, type: pointer to any 

'arguments' is a local var to all functions, representing a pseudo-array witj all the arguments.
'arguments' has only one method: `arguments.toArray()`

we declare here the type:"pointer to any" - "any*"

        var argumentsType = globalScope.addMember('any*',{nodeClass:Grammar.ClassDeclaration}) //  any pointer, type for "arguments"

-"arguments" have only one method: "toArray()"

        argumentsType.addMember('toArray',{
              type:       globalPrototype('Function')
              returnType: globalPrototype('Array')
              nodeClass:  Grammar.FunctionDeclaration
          }) 

b.2) Lite-C: the Lexer replaces string interpolation with calls to `_concatAny`

        globalScope.addMember '_concatAny',{ //used for string interpolation
              type:       globalPrototype('Function')
              returnType: globalPrototype('String')
              nodeClass:  Grammar.FunctionDeclaration
        } 

        globalScope.addMember 'parseFloat',{
              type:       globalPrototype('Function')
              returnType: globalPrototype('Number')
              nodeClass:  Grammar.FunctionDeclaration
              }

        globalScope.addMember 'parseInt',{
              type:       globalPrototype('Function')
              returnType: globalPrototype('Number')
              nodeClass:  Grammar.FunctionDeclaration
              }

        globalScope.addMember '__dirname',{ // current module dir (node.js)
              type:       globalPrototype('String')
              nodeClass:  Grammar.VariableDecl
              } 

Process the global scope declarations interface file
by parsing the file: "interfaces/GlobalScope(JS|C|NODE|BROWSER).interface.md"

        processInterfaceFile 'GlobalScope#{project.options.target.toUpperCase()}'

if we're compiling for node.js, add extra node global core objects, e.g: process, Buffer
if we're compiling for the browser, add window, document the DOM, localStorage,etc

        if project.options.target is 'js'
            if project.options.browser
                processInterfaceFile 'GlobalScopeBROWSER'
            else
                processInterfaceFile 'GlobalScopeNODE'

Initial NameAffinity, err|xxxErr => type:Error

        if tryGetGlobalPrototype('Error') into var errProto:Names.Declaration 
            nameAffinity.members.set 'Err',errProto.parent // err|xxxErr => type:Error

### helper function processInterfaceFile(globalInterfaceFile)

Process the global scope declarations interface file: GlobalScope(JS|C|NODE).interface.md

        logger.info 'Declare global scope using #{globalInterfaceFile}.interface.md'
        var globalInterfaceModule = project.compileFile(globalInterfaceFile)
        //logger.info '    from:',globalInterfaceModule.fileInfo.relFilename

call "declare" on each item of the GlobalScope interface file, to create the Names.Declaration
(normally in the global scope)

        globalInterfaceModule.callOnSubTree LiteCore.getSymbol('declare')

/*move all exported from the interface file, to project.rootModule global scope

        for each nameDecl in map globalInterfaceModule.exports.members
            project.rootModule.addToSpecificScope globalScope, nameDecl
*/

----------

## Module Helper Functions

### Helper function tryGetGlobalPrototype(name) 
gets a var from global scope
      
      if globalScope.findOwnMember(name) into var nameDecl
          return nameDecl.members.get("prototype")

### public Helper function globalPrototype(name) 
gets a var from global scope

      if name instanceof Names.Declaration, return name #already converted type

      if not globalScope.findOwnMember(name) into var nameDecl
        fail with "no '#{name}' in global scope"

      if no nameDecl.findOwnMember("prototype") into var protoNameDecl
        fail with "global scope type '#{name}' must have a 'prototype' property"

      return protoNameDecl


### helper function addBuiltInClass(name,node) returns Names.Declaration
Add a built-in class to global scope, return class prototype

      var nameDecl = new Names.Declaration( name, {nodeClass:Grammar.ClassDeclaration} ,node )
      globalScope.addMember nameDecl

      nameDecl.getMembersFromObjProperties Environment.getGlobalObject(name)

      if no nameDecl.findOwnMember("prototype") into var classProto:Names.Declaration
          throw new Error("addBuiltInClass '#{name}, expected to have a prototype")

      nameDecl.setMember '**proto**', globalPrototype('Function')
      // commented v0.8: classes can not be used as functions. 
      // nameDecl.setMember '**return type**', classProto

      return classProto


### helper function addBuiltInObject(name,node) returns Names.Declaration
Add a built-in object to global scope, return object

      var nameDecl = new Names.Declaration(name, {nodeClass:Grammar.NamespaceDeclaration}, node)
      globalScope.addMember nameDecl
      nameDecl.getMembersFromObjProperties Environment.getGlobalObject(name)

      if nameDecl.findOwnMember("prototype") 
          throw new Error("addBuiltObject '#{name}, expected *Object* to have not a prototype")

      return nameDecl

---------------------------------------
----------------------------------------
----------------------------------------

### Append to namespace Names

      class ConvertResult
        properties
          converted:number=0
          failures:number=0

##Additions to Names.Declaration. Helper methods to do validation

### Append to class Names.Declaration

#### Helper method findMember(name) returns Names.Declaration
this method looks for a name in Names.Declaration members,
it also follows the **proto** chain (same mechanism as js __proto__ chain)

        var actual = this
        var count=0

        do while actual instance of Names.Declaration 

            if actual.findOwnMember(name) into var result
               return result

We use a member named '**proto**' on NameDeclarations, mapping run-time proto chain.
We follow the chain to validate property names.

            var nextInChain = actual.findOwnMember('**proto**')

As last option in the chain, we always use 'Object.prototype'

            if no nextInChain and actual isnt globalObjectProto
              nextInChain = globalObjectProto

            actual = nextInChain

            if count++ > 50 #assume circular
                .warn "circular type reference"
                return
        
        loop


#### Helper method hasProto(name) returns boolean
this method looks for a name in Names.Declaration members **proto**->prototype->parent
it also follows the **proto** chain (same mechanism as js __proto__ chain)

        var actual = this
        var count=0

        do while actual instance of Names.Declaration 

            if actual.name is 'prototype' and actual.parent.name is name
                return true

We use a member named '**proto**' on NameDeclarations, mapping run-time proto chain.
We follow the chain to validate property names.

            var nextInChain = actual.findOwnMember('**proto**')

As last option in the chain, we always use 'Object.prototype'

            if no nextInChain and actual isnt globalObjectProto
                nextInChain = globalObjectProto

            actual = nextInChain

            if count++ > 50 #assume circular
                .warn "circular type reference"
                return
        
        loop

#### Helper Method getMembersFromObjProperties(obj) #Recursive
Recursively converts a obj properties in NameDeclarations.
it's used when a pure.js module is imported by 'require'
to convert required 'exports' to LiteScript compiler usable NameDeclarations
Also to load the global scope with built-in objects

        #ifdef TARGET_C
        return
        #else
        var newMember:Names.Declaration

        if obj instanceof Object or obj is Object.prototype
            declare obj:array //to allow js's property access []
            for each prop in Object.getOwnPropertyNames(obj) #even not enumerables
                where prop not in ['__proto__','arguments','caller'] #exclude __proto__ 

                    var type =  Grammar.autoCapitalizeCoreClasses(typeof obj[prop])
                    type = tryGetGlobalPrototype(type) #core classes: Function, Object, String
                    if type is this, type = undefined #avoid circular references

                    newMember = .addMember(prop,{type:type})

on 'prototype' member or 
if member is a Function-class - dive into

                    declare valid Object.hasOwnProperty.call
                    if prop isnt 'constructor' 
                        if  prop is 'prototype' 
                            or (typeof obj[prop] is 'function' 
                                and obj[prop].hasOwnProperty('prototype') 
                                and not .isInParents(prop) 
                               )
                            or (typeof obj[prop] is 'object' 
                                and not .isInParents(prop) 
                               )
                              newMember.getMembersFromObjProperties(obj[prop]) #recursive
                              if prop is 'super_' # used in node's core modules: http, EventEmitter, etc.
                                  if newMember.findOwnMember('prototype') into var superProtopNameDecl 
                                    var protopNameDecl = .findOwnMember('prototype') or .addMember('prototype')
                                    protopNameDecl.setMember '**proto**', superProtopNameDecl #put super's proto in **proto** of prototype

        #end if

                        

#### Helper Method isInParents(name)
return true if a property name is in the parent chain.
Used to avoid recursing circular properties
        
        var nameDecl = this.parent
        while nameDecl
          if nameDecl.findOwnMember(name), return true
          nameDecl = nameDecl.parent


#### Helper method processConvertTypes(options:Names.DeclarationOptions) returns Names.ConvertResult
convert possible types stored in Names.Declaration, 
from string/varRef to other NameDeclarations in the scope

        var result = new Names.ConvertResult

        .convertType '**proto**',result,options  #try convert main type
        .convertType '**return type**',result,options  #a Function can have **return type**
        .convertType '**item type**',result,options  #an Array can have **item type** e.g.: 'var list: string array'

        return result


#### Helper method convertType(internalName, result: Names.ConvertResult, options: Names.DeclarationOptions) 
convert type from string to NameDeclarations in the scope.
returns 'true' if converted, 'false' if it has to be tried later

        if no .findOwnMember(internalName) into var typeRef
            #nothing to process
            return  

        if typeRef instance of Names.Declaration
            #already converted, nothing to do
            return 

        if typeRef instance of Grammar.TypeDeclaration
            declare valid typeRef.mainType
            typeRef = typeRef.mainType

        var converted:Names.Declaration

        # if the typeRef is a varRef, get reference 
        if typeRef instanceof Grammar.VariableRef
            declare typeRef:Grammar.VariableRef
            converted = typeRef.tryGetReference()

        else if typeof typeRef is 'string' #built-in class or local var

            if no .nodeDeclared #converting typeRef for a var not declared in code
              converted = globalPrototype(typeRef)
            else
              converted = .nodeDeclared.findInScope(typeRef)
            end if

        else
            declare valid typeRef.constructor.name
            .sayErr "INTERNAL ERROR: convertType UNRECOGNIZED type of:'#{typeof typeRef}' on #{internalName}: '#{typeRef}' [#{typeRef.constructor.name}]"
            return

        end if #check instance of "typeRef"


        if converted
            #move to prototype if referenced is a class
            if converted.findOwnMember("prototype") into var prototypeNameDecl
                converted = prototypeNameDecl
            #store converted
            .setMember(internalName,converted)
            result.converted++
        else
            result.failures++
            if options and options.informError, .sayErr "Undeclared type: '#{typeRef.toString()}'"
        end if

        return 


#### helper method assignTypeFromValue(value) 
if we can determine assigned value type, set var type

      declare valid value.getResultType:function
      var valueNameDecl = value.getResultType()

now set var type (unless is "null" or "undefined", because they destroy type info)

      if valueNameDecl instance of Names.Declaration 
        and valueNameDecl.name not in ["undefined","null"]

            var theType
            if valueNameDecl.name is 'prototype' # getResultType returns a class prototype
                // use the class as type
                theType = valueNameDecl
            else 
                //we assume valueNameDecl is a simple var, then we try to get **proto**
                theType = valueNameDecl.findOwnMember('**proto**') or valueNameDecl
            end if

            // assign type: now both nameDecls points to same type
            .setMember '**proto**', theType 



#### helper method assignTypebyNameAffinity() 
Auto-assign type by name affinity. 
If no type specified, check project.nameAffinity
        
        if .nodeDeclared and not String.isCapitalized(.name) and .name isnt 'prototype'

            if not .findOwnMember('**proto**')

                var possibleClassRef:Names.Declaration
                # possibleClassRef is a Names.Declaration whose .nodeDeclared is a ClassDeclaration

                #should look as className. Also when searching with "endsWith", 
                # nameAffinity declarations are stored capitalized
                var asClassName = .name.capitalized()

                # look in name affinity map
                if no nameAffinity.members.get(.name) into possibleClassRef
                    # make first letter uppercase, e.g.: convert 'lexer' to 'Lexer'
                    # try with name, 1st letter capitalized
                    possibleClassRef = nameAffinity.members.get(asClassName) 
                end if
                
                # check 'ends with' if name is at least 6 chars in length
                if not possibleClassRef and .name.length>=6
                    for each affinityName,classRef in map nameAffinity.members
                        if asClassName.endsWith(affinityName)
                            possibleClassRef = classRef
                            break

                #if there is a candidate class, check of it has a defined prototype
                if possibleClassRef and possibleClassRef.findOwnMember("prototype") into var prototypeNameDecl
                      .setMember '**proto**', prototypeNameDecl
                      return true


--------------------------------
## Helper methods added to AST Tree

### Append to class ASTBase

#### properties
        scope: Names.Declaration //for nodes with scope

#### helper method declareName(name, options:Names.DeclarationOptions) 
declareName creates a new Names.Declaration, *referecing source as nodeDeclared (AST node)*

        return new Names.Declaration(name, options, this)

#### method addMemberTo(nameDecl, memberName, options:Names.DeclarationOptions)  returns Names.Declaration
a Helper method ASTBase.*addMemberTo*
Adds a member to a NameDecl, referencing this node as nodeDeclared
        
        return nameDecl.addMember(memberName, options, this)

#### Helper method tryGetMember(nameDecl, name:string, options:Names.DeclarationOptions)
this method looks for a specific member, optionally declare as forward
or inform error. We need this AST node, to correctly report error.
        
        default options = new Names.DeclarationOptions
  
        var found = nameDecl.findMember(name)
        
        if found and name.slice(0,2) isnt '**'
          found.caseMismatch name,this
        
        else #not found

          if options.informError 
                logger.warning "#{.positionText()}. No member named '#{name}' on #{nameDecl.info()}"
                if nameDecl.nodeDeclared, nameDecl.nodeDeclared.warn "declaration is here"
          
          if options.isForward, found = .addMemberTo(nameDecl,name,options)

        return found


#### helper method getScopeNode(stopAtAppendTo:boolean) 

**getScopeNode** method return the parent 'scoped' node in the hierarchy.
It looks up until found a node with .scope
        
Start at this node

        var node = this

        while node

          if node.scope or (stopAtAppendTo and node.constructor is Grammar.AppendToDeclaration)
              return node # found a node with scope | Grammar.AppendToDeclaration

          node = node.parent # move up

        #loop

        return null


#### method findInScope(name) returns Names.Declaration
this method looks for the original place 
where a name was defined (function,method,var) 
Returns the Identifier node from the original scope
It's used to validate variable references to be previously declared names

Start at this node

        var node = this 
        var found

Look for the declaration in this scope

        while node

          if node.scope

              if node.scope.findOwnMember(name) into found
                  return found

move up in scopes

          node = node.parent

        #loop

In JS the global environment (global|window) is a *Object*, and as such it 
*has* Object.prototype in its prototype chain, which means 
*all properties in Object.prototype are also in the global scope*

**To emulate JS (quirky) behavior, if a name is not found in any scope up to global scope, 
we must search also Object.prototype (since is __proto__ of global scope object).
This help alleviating subtle bugs in js, if tou dare to add something to Object.prototype.

        if globalObjectProto.findOwnMember(name) into found
            return found


#### method tryGetFromScope(name, options:Names.DeclarationOptions) returns Names.Declaration
a Helper method: *ASTBase.tryGetFromScope(name)*, this method looks for the original declaration
in the scope. if the declaration is not found, an error is emmited and a -pseudo- var is created 
in the scope in order to continue compilation

Check if the name is declared. Retrieve the original declaration

if it's already a Names.Declaration, no need to search

        if name instanceof Names.Declaration, return name  

Search the scope

        if .findInScope(name) into var found 

Declaration found, we check the upper/lower case to be consistent
If the found item has a different case than the name we're looking for, emit error 

            if found.caseMismatch(name, this)
                return found
            end if

if it is not found,check options: a) inform error. b) declare foward.

        else
            if options and options.informError
                .sayErr "UNDECLARED NAME: '#{name}'"

            if options and options.isForward
                found = .addToScope(name,options)  
                if options.isDummy and String.isCapitalized(name) #let's assume is a class
                    .addMemberTo(found,'prototype',options)

        #end if - check declared variables 

        return found


#### method addToScope(item, options:Names.DeclarationOptions) returns Names.Declaration 
a Helper method ASTBase.*addToScope*
Search for parent Scope, adds passed name to scope.members
Reports duplicated.
return: Names.Declaration

        if no item, return # do nothing on undefined params

        var scope:Names.Declaration = .getScopeNode().scope

        return .addToSpecificScope(scope, item, options)

First search it to report duplicates, if found in the scope.
If the found item has a different case than the name we're adding, emit error & return

#### method addToSpecificScope(scope:Names.Declaration, item, options:Names.DeclarationOptions) returns Names.Declaration 

        declare valid item.name
        var name = type of item is 'string'? item : item.name

        logger.debug "addToScope: '#{name}' to '#{scope.name}'" #[#{.constructor.name}] name:

        if .findInScope(name) into var found 

            if found.caseMismatch(name, this)
                #case mismatch informed
                do nothing

            else if found.isForward
                found.isForward = false
                found.nodeDeclared = this
                if item instanceof Names.Declaration
                    found.replaceForward item

            else 
                var errPosNode:ASTBase = item instanceof Names.Declaration? item else this
                errPosNode.sayErr "DUPLICATED name in scope: '#{name}'"
                logger.error found.originalDeclarationPosition() #add extra information line

            return found

        #end if

else, not found, add it to the scope

        var nameDecl
        if item instanceof Names.Declaration
          nameDecl = item
        else
          nameDecl = .declareName(name,options)

        scope.addMember nameDecl

        return nameDecl


#### method selectAndAddToScope(nameDecl)

if has adjective "global" add to global scope

        if .hasAdjective('global')
            globalScope.addMember nameDecl

get parent. We cover here class/namespaces directly declared inside namespaces (without AppendTo)
        
        else
            var container = .parent.getScopeNode(stopAtAppendTo=true)

if it is declared inside a namespace, it is added to namespace scope, so inside a namespace, 
namespace functions and properties can be accessed directly.
*AFTER* it is added as a item of the namespace (as a Object).
This need to be done *after* so the nameDecl.parent ends up being the namespace rather than "namespace scope"

            if container.constructor is Grammar.NamespaceDeclaration
                declare container: Grammar.NamespaceDeclaration
                .addToSpecificScope container.scope, nameDecl //1st
                container.nameDecl.addMember nameDecl //2nd - order is important (to set parent)

            else if container.constructor is Grammar.AppendToDeclaration
                do nothing //will be handled later in processAppendToExtends()
            else
                //else, another kind of container, 
                // it could be a internal-function inside a function, a module-level class|namespace|fn|var. 
                //Add to parent scope
                .addToSpecificScope container.scope, nameDecl 

export:
if has adjective public/export, add to module.exports

            if .hasAdjective('export') 
                if container.constructor isnt Grammar.Module, .sayErr "only module-level objects can be exported. '#{nameDecl}' is contained in #{container} '#{container.name}'."
                var moduleNode:Grammar.Module = .getParent(Grammar.Module)
                moduleNode.addToExport nameDecl 


#### Helper method createScope()
initializes an empty scope in this node

        if no .scope 

            var scopeName = .name is '*Global Scope*'? .name else "[#{.constructor.name} #{.name} Scope]"

            .scope = .declareName(scopeName, {
                  normalizeModeKeepFirstCase:true
                  nodeClass: Grammar.VariableDecl
                  })

            .scope.isScope = true

        return .scope

#### helper method tryGetOwnerNameDecl( informError ) returns Names.Declaration

Returns namedeclaration where this node should be.
Used for properties & methods declarations.
If the parent is Append-To, search for the referenced clas/namespace.

returns owner.nameDecl or nothing

        var toNamespace
        var ownerDecl 

        # get parent ClassDeclaration/Append-to/Namespace
        var parent:Grammar.ClassDeclaration = .getParent(Grammar.ClassDeclaration)

        if no parent
           if informError, .throwError "declaration is outside 'class/namespace/append to'. Check indent"
           return          

Append to class|namespace

        if parent instance of Grammar.AppendToDeclaration

            #get varRefOwner from AppendToDeclaration
            declare parent:Grammar.AppendToDeclaration

            toNamespace = parent.toNamespace #if it was 'append to namespace'

            #get referenced class/namespace
            if no parent.varRef.tryGetReference() into ownerDecl
                if informError 
                    .sayErr "Append to: '#{parent.varRef}'. Reference is not fully declared"
                return //if no ownerDecl found

        else # directly inside a ClassDeclaration|NamespaceDeclaration

            toNamespace = parent.constructor is Grammar.NamespaceDeclaration

            ownerDecl = parent.nameDecl
            if no ownerDecl

                if parent.hasAdjective('shim') // it was a shim class|namespace
                    ownerDecl = .findInScope(parent.name) //get pre-existent

                if no ownerDecl
                    return .sayErr("cannot get parent name declaration")

        end if


check if owner is class (namespace) or class.prototype (class)


        if toNamespace 
            #'append to namespace'/'namespace x'. Members are added directly to owner
            return ownerDecl

        else
            # Class: members are added to the prototype
            # move to class prototype
            if no ownerDecl.findOwnMember("prototype") into var ownerDeclProto
                if informError, .sayErr "Class '#{ownerDecl}' has no .prototype"
                return

            # Class: members are added to the prototype
            return ownerDeclProto

        end if


#### helper method callOnSubTree(methodSymbol,excludeClass) # recursive

This is instance has the method, call the method on the instance

      //logger.debugGroup "callOnSubTree #{.constructor.name}.#{LiteCore.getSymbolName(methodSymbol)}() - '#{.name}'"
  
      if this.tryGetMethod(methodSymbol) into var theFunction:Function 
            logger.debug "calling #{.constructor.name}.#{LiteCore.getSymbolName(methodSymbol)}() - '#{.name}'"
            theFunction.call(this)

      if excludeClass and this is instance of excludeClass, return #do not recurse on filtered's childs

recurse on all properties (exclude 'parent' and 'importedModule' and others, shortcut-references)

      for each property name,value in this
        where name not in ['constructor','parent','importedModule','requireCallNodes','constructorDeclaration']

            if value instance of ASTBase 
                declare value:ASTBase
                value.callOnSubTree methodSymbol,excludeClass #recurse

            else if value instance of Array
                declare value:array 
                //logger.debug "callOnSubArray #{.constructor.name}.#{name}[]"
                for each item in value where item instance of ASTBase
                    declare item:ASTBase
                    item.callOnSubTree methodSymbol,excludeClass
      end for

      //logger.debugGroupEnd



### Append to class Grammar.Module ###

#### Helper method addToExport(exportedNameDecl)

Add to parentModule.exports, but *preserve parent*
      
      #just add to actual exports, but preserve parent
      var saveParent = exportedNameDecl.parent

      .exports.addMember(exportedNameDecl)

      exportedNameDecl.parent = saveParent


#### Helper method confirmExports()

Check that:
- if we have a "default export" (a class/namespace named as the module)
  - we cannot have other vars or functions declared public/export
  - replace module.exports with the default export object
  
      var exportDefaultNameDecl

search for a export default object (a class/namespace named as the module)

      for each nameDecl in map .exports.members
          if nameDecl.nodeDeclared and nameDecl.nodeDeclared.hasAdjective('only export') 
              exportDefaultNameDecl = nameDecl
              break

      if exportDefaultNameDecl

          if .exports.getMemberCount() > 1
              //only one "export-only" allowed 
              for each nameDecl in map .exports.members 
                where nameDecl isnt exportDefaultNameDecl and nameDecl.parent isnt exportDefaultNameDecl
                  nameDecl.warn 'only export: cannot have "public functions/vars" and also a *only export* class/namespace'

          //set as namespace & replace module.exports
          .exports.makePointTo exportDefaultNameDecl
          .exports.name = exportDefaultNameDecl.name
          .exportsReplaced = true


----
## Methods added to specific Grammar Classes to handle scope, var & members declaration

### Append to class Grammar.VariableDecl ###

`VariableDecl: Identifier (':' dataType-IDENTIFIER) ('=' assignedValue-Expression)`

variable name, optional type anotation and optionally assign a value

VariableDecls are used in:
1. `var` statement
2. function *parameter declaration* 
3. class *properties declaration*

Examples:  
  `var a : string = 'some text'` 
  `function x ( a : string = 'some text', b, c=0)`

      properties nameDecl

      helper method createNameDeclaration()  
        declare .type: Grammar.TypeDeclaration
        return .declareName(.name,{type:.type})

      helper method declareInScope()  
          .nameDecl = .addToScope(.createNameDeclaration())

      helper method connectAlias()  
          if .aliasVarRef
              //Example: "public var $ = jQuery" => declare alias $ for jQuery
              if .aliasVarRef.tryGetReference({informError:true}) into var ref
                  # aliases share .members
                  .nameDecl.members = ref.members

      helper method getTypeFromAssignedValue() 

          // if it has an assigned value
          if .nameDecl and .assignedValue and .nameDecl.name isnt 'prototype'
              
              if .assignedValue instanceof Grammar.Expression
                and .assignedValue.root.name.constructor in [Grammar.StringLiteral,Grammar.NumberLiteral]
                    var theLiteral = .assignedValue.root.name
                    // if it is assigning a literal, force type to string|number|array
                    .nameDecl.setMember('**proto**', globalPrototype(theLiteral.type))

              else if no .nameDecl.findOwnMember('**proto**') into var type #if has no type
                  if .assignedValue.getResultType() into var result #get assignedValue type
                      .nameDecl.setMember('**proto**', result) #assign to this.nameDecl


### Append to class Grammar.VarStatement ###

     method declare()  # pass 1

        var moduleNode:Grammar.Module = .getParent(Grammar.Module)
        var isPublic = .hasAdjective("export")
        var isGlobal = .hasAdjective("global")

        for each varDecl in .list

            if isGlobal
                varDecl.nameDecl = varDecl.createNameDeclaration()
                globalScope.addMember varDecl.nameDecl
            else
                varDecl.declareInScope

                if isPublic
                    
                    moduleNode.addToExport varDecl.nameDecl

                    //mark as isPublicVar to prepend "module.exports.x" when referenced in module body
                    // except interfaces (no body & vars are probably aliases. case: public var $=jQuery)
                    if not moduleNode.fileInfo.isInterface
                          varDecl.nameDecl.isExported = true
                

     method evaluateAssignments() # pass 4, determine type from assigned value
        for each varDecl in .list
            varDecl.getTypeFromAssignedValue


### Append to class Grammar.WithStatement ###

      properties nameDecl

      method declare()  # pass 1
         .nameDecl = .addToScope(.declareName(.name))

      method evaluateAssignments() # pass 4, determine type from assigned value
        .nameDecl.assignTypeFromValue .varRef
      

### Append to class Grammar.ImportStatementItem ###

      properties nameDecl

      method declare #pass 1: declare name choosen for imported(required) contents as a scope var

        if no .getParent(Grammar.DeclareStatement) #except for 'global declare'

            if .hasAdjective('shim') and .findInScope(.name), return // do not import if shim and already declared

            .nameDecl = .addToScope(.name)


----------------------
### Append to class Grammar.ClassDeclaration 
derived classes are:  AppendToDeclaration and NamespaceDeclaration 

#### properties
      nameDecl

#### method declare() # class

if is a class adjectivated "shim", do not declare if already exists
    
        if .hasAdjective('shim') 
            if .tryGetFromScope(.name) 
                return 

        .nameDecl = .declareName(.name, {type:globalPrototype('Function')} ) //class
        
        .selectAndAddToScope .nameDecl

we create 'Class.prototype' member
Class's properties & methods will be added to 'prototype' as valid member members.
'prototype' starts with 'constructor' which is a pointer to the class-funcion itself

        var prtypeNameDecl = .nameDecl.findOwnMember('prototype') or .addMemberTo(.nameDecl,'prototype')
        if .varRefSuper 
            prtypeNameDecl.setMember('**proto**',.varRefSuper)
        //else
        //    prtypeNameDecl.setMember('**proto**',globalObjectProto)

        prtypeNameDecl.addMember('constructor',{pointsTo:.nameDecl}) 

return type of the class-function, is the prototype

        .nameDecl.setMember '**return type**',prtypeNameDecl

add to nameAffinity

        if not nameAffinity.members.has(.name)
            nameAffinity.members.set .name, .nameDecl


#### method validatePropertyAccess() 

in the pass "Validating Property Access", for a "ClassDeclaration"
we check for duplicate property names in the super-class-chain

        if .constructor isnt Grammar.ClassDeclaration, return // exclude derived classes

        var prt:Names.Declaration = .nameDecl.ownMember('prototype')
        for each propNameDecl in map prt.members where propNameDecl.nodeClass is Grammar.VariableDecl
                propNameDecl.checkSuperChainProperties .nameDecl.superDecl


### Append to class Grammar.NamespaceDeclaration
#### method declare()

        if .hasAdjective('shim') 
            if .tryGetFromScope(.name) 
                return 

declare the namespace. namespaces also create a scope

        .nameDecl = .declareName(.name)
        .createScope

        .selectAndAddToScope .nameDecl

   
### Append to class Grammar.AppendToDeclaration
#### method declare()
AppendToDeclarations do not "declare" anything at this point. 

AppendToDeclarations add to a existing classes or namespaces. 
The adding is delayed until pass:"processAppendToExtends", 
where append-To var reference is searched in the scope 
and methods and properties are added. 
This need to be done after all declarations.
    
      do nothing

----------------------------

### Append to class Grammar.ArrayLiteral ###

     properties nameDecl

     method declare

When producing C-code, an ArrayLiteral creates a "new(Array" at module level

        if project.options.target is 'c'
            .nameDecl = .declareName(UniqueID.getVarName('_literalArray'))
            .getParent(Grammar.Module).addToScope .nameDecl
        
     method getResultType
          return tryGetGlobalPrototype('Array')


### Append to class Grammar.ObjectLiteral ###

     properties nameDecl

     method declare

When producing js-code, an ObjectLiteral declares a new ad-hoc "type". 
The var is assigned this ad-hoc "type"

When producing c-code, an ObjectLiteral creates a "Map string to any" on the fly, 
so *it does not declare a type with members*

        if project.options.target is 'js'

if the .parent has a .nameDecl we copy that so members get added there.
if it does not, create a new one (we're a interal LiteralObject, 'value'
of a name:value pair)

          .nameDecl = .parent.tryGetProperty('nameDecl') 

          if no .nameDecl
              .nameDecl = .declareName(UniqueID.getVarName('*ObjectLiteral*'))

When producing the LiteScript-to-C compiler, a ObjectLiteral's return type is 'Map string to any'

When producing js-code is the ad-hoc type created for the ObjectLiteral

     method getResultType

        if project.options.target is 'c' 
            return tryGetGlobalPrototype('Map')
        else
            return  .nameDecl


### Append to class Grammar.NameValuePair ###
    
     properties nameDecl

     method declare

When producing C-code, a ObjectLiteral creates a "Map string to any" on the fly, 
but it does not declare a valid type/class.

        if project.options.target is 'c', return

Add this name as member of the parent ObjectLiteral/Value
        
        declare .parent: Grammar.ObjectLiteral 
        .nameDecl = .addMemberTo(.parent.nameDecl, .name)

check if we can determine type from value 

        if .type 
            .nameDecl.setMember '**proto**', .type

        else if .value
            .nameDecl.assignTypeFromValue .value

### Append to class Grammar.FunctionDeclaration ###
`FunctionDeclaration: '[export][generator] (function|method|constructor) [name] '(' FunctionParameterDecl* ')' Block`

     properties 
        nameDecl 
        declared:boolean

#### Method declare() ## function, methods and constructors

      var ownerNameDecl
      var isMethod = .constructor is Grammar.MethodDeclaration
      var isFunction = .constructor is Grammar.FunctionDeclaration

1st: Grammar.FunctionDeclaration

if not anonymous, add function name to parent scope,

      if isFunction 
          if .name
              .nameDecl = .declareName(.name)
              .selectAndAddToScope .nameDecl

2nd: Methods & constructors

Try to determine ownerNameDecl, to declare the memeber and to set scope var "this"'s **proto**.
if ownerNameDecl *can* be determined at this point, declare method as member.

      ownerNameDecl = .tryGetOwnerNameDecl() 

      if isMethod and .name and ownerNameDecl

          .addMethodToOwnerNameDecl ownerNameDecl

      end if // function or method 

Note: following JS design, constructors are the body of the function-class itself,
so the "constructor" is not a member function

Define function's return type from parsed text

      var returnType = .createReturnType()

Functions (methods and constructors also), have a 'scope'. 
It captures al vars declared in its body.
We now create function's scope and add the special var 'this'. 
The 'type' of 'this' is normally a class prototype, 
which contains other methods and properties from the class.
We also add 'arguments.length'

Scope starts populated by 'this' and 'arguments'.

      var scope = .createScope()

      .addMemberTo scope,'arguments', {type:'any*', nodeClass:Grammar.VariableDecl}

      
      // NOTE: in js there's a "this" everywhere. In browser mode,
      // "this" on a global function is normally used when such function is registered as 
      // a DOM node event handler (this=DOM node triggering the event)
      
      var typeOfThis

      if isFunction 
          //for "functions" add a "this" without type
          do nothing
      
      else
          //for "methods", "this" :type is the class prototype

          if no .getParent(Grammar.ClassDeclaration) into var containerClassDeclaration //also append-to & NamespaceDeclaration
              .sayErr "method outside class|namespace|apeend-to"
              return

          if containerClassDeclaration.constructor is Grammar.ClassDeclaration
              typeOfThis = ownerNameDecl

          else if containerClassDeclaration.constructor is Grammar.AppendToDeclaration
              declare containerClassDeclaration:Grammar.AppendToDeclaration
              typeOfThis = containerClassDeclaration.varRef

      end if //select typeOfThis

      .addMemberTo(scope,'this',{type:typeOfThis,nodeClass:Grammar.VariableDecl})

Note: only class methods have 'this' as parameter

add parameters to function's scope

      if .paramsDeclarations
          for each varDecl in .paramsDeclarations.list
              varDecl.declareInScope


#### helper method addMethodToOwnerNameDecl(owner:Names.Declaration)  ## methods

      var actual = owner.findOwnMember(.name)

      if actual and .hasAdjective('shim') #shim for an exising method, do nothing
        return

Add to owner, type is 'Function'

      if no .nameDecl 
          .nameDecl = .declareName(.name,{type:globalPrototype('Function')})
      
      .declared = true

      .addMemberTo owner, .nameDecl


#### method createReturnType() ## functions & methods

      if no .nameDecl, return #nowhere to put definitions

      .nameDecl.setMember "**proto**", globalPrototype('Function')

Define function's return type from parsed text

      if .type and .type.itemType

if there's a "itemType", it means type is: `array of [itemType]`
We create a intermediate type for `Array of itemType` 
and set this new nameDecl as function's **return type**

          var composedName = 'Array of #{.type.itemType.toString()}'

check if it already exists, if not found, create one. Type is 'Array'
        
          if not globalScope.findMember(composedName) into var intermediateNameDecl
              intermediateNameDecl = globalScope.addMember(composedName, {
                    type:globalPrototype('Array')
                    nodeClass:Grammar.ClassDeclaration
                    })

item type, is each array member's type 

          intermediateNameDecl.setMember "**item type**", .type.itemType

          .nameDecl.setMember '**return type**', intermediateNameDecl

else, it's a simple type

      else 

          if .type, .nameDecl.setMember('**return type**', .type)


### Append to class Grammar.AppendToDeclaration ###

#### method processAppendToExtends() 
    
get referenced class/namespace

      if no .varRef.tryGetReference() into var ownerDecl
          .sayErr "Append to: '#{.varRef}'. Reference is not fully declared"
          return //if no ownerDecl found

      if not .toNamespace
          //if is "append to class"
          if no ownerDecl.findOwnMember('prototype') into var prt 
              .throwError "Append to: class '#{ownerDecl}' has no prototype"

          ownerDecl=prt // append to class, adds to prototype

      //if project.options.target is 'c'
      //    if .toNamespace and prt
      //        .sayErr "Append to: '#{.varRef}'. For C production, cannot append to class as namespace."

      for each item in .body.statements

          case item.specific.constructor

Add all properties as members of its owner (append to).
For undeclared properties only

              when Grammar.PropertiesDeclaration:
                  declare item.specific:Grammar.PropertiesDeclaration
                  if not item.specific.declared, item.specific.declare(informError=true) 

For undeclared methods only

              when Grammar.MethodDeclaration:
                  var m:Grammar.MethodDeclaration = item.specific
                  if m.declared, continue

Now that we have 'owner' we can set **proto** for scope var 'this', 
so we can later validate `.x` in `this.x = 7`

                  m.addMethodToOwnerNameDecl ownerDecl

                  if m.scope.findOwnMember("this") into var scopeThis 
                      scopeThis.setMember '**proto**',ownerDecl
                      #set also **return type**
                      m.createReturnType

a class appended to a namespace

              when Grammar.ClassDeclaration:
                  declare item.specific:Grammar.ClassDeclaration
                  ownerDecl.addMember item.specific.nameDecl                 

              when Grammar.EndStatement:
                  do nothing

              else
                  .sayErr 'unexpected "#{item.specific.constructor.name}" inside Append-to Declaration'


### Append to class Names.Declaration ###
#### Properties 
      superDecl : Names.Declaration //nameDecl of the super class

#### method checkSuperChainProperties(superClassNameDecl)

        if no superClassNameDecl, return 

Check for duplicate class properties in the super class

        if superClassNameDecl.findOwnMember('prototype') into var superPrt:Names.Declaration
            
            if superPrt.findOwnMember(.name) into var originalNameDecl
                .sayErr "Duplicated property. super class [#{superClassNameDecl}] already has a property '#{this}'"
                originalNameDecl.sayErr "for reference, original declaration."

recurse with super's super. Here we're using recursion as a loop device à la Haskell
(instead of a simpler "while .superDecl into node" loop. Just to be fancy)

            .checkSuperChainProperties superClassNameDecl.superDecl

### Append to class Grammar.ClassDeclaration ###

#### method processAppendToExtends() 
In Class's processAppendToExtends we try to get a reference to the superclass
and then store the superclass nameDecl in the class nameDecl
    
get referenced super class

      if .varRefSuper 
          if no .varRefSuper.tryGetReference() into var superClassNameDecl
              .sayErr "class #{.name} extends '#{.varRefSuper}'. Reference is not fully declared"
              return //if no superClassNameDecl found

          .nameDecl.superDecl = superClassNameDecl

### Append to class Grammar.PropertiesDeclaration ###

     properties 
        nameDecl 
        declared:boolean 

#### method declare(informError) 
Add all properties as members of its owner object (normally: class.prototype)

        if .tryGetOwnerNameDecl(informError) into var ownerNameDecl 

            for each varDecl in .list
                varDecl.nameDecl = varDecl.addMemberTo(ownerNameDecl,varDecl.name,{type:varDecl.type})
            end for

            .declared = true

#### method evaluateAssignments() # determine type from assigned value on properties declaration

        for each varDecl in .list
            varDecl.getTypeFromAssignedValue



### Append to class Grammar.ForStatement ###

#### method declare()

a ForStatement has a 'Scope', keyIndexVar & valueVar belong to the scope

        .createScope

### Append to class Grammar.ForEachProperty ###

#### method declare()

        if .iterable.type
            default .valueVar.type = .iterable.type.itemType

        .valueVar.declareInScope

        if .keyIndexVar, .keyIndexVar.declareInScope

#### method evaluateAssignments() 

ForEachProperty: index is: string for js (property name) and number for C (symbol)

        if .keyIndexVar
        
            var indexType = project.options.target is 'js'? 'String':'Number'
            .keyIndexVar.nameDecl.setMember('**proto**',globalPrototype(indexType))

### Append to class Grammar.ForEachInArray ###

#### method declare()

        if .iterable.type
            default .valueVar.type = .iterable.type.itemType

        .valueVar.declareInScope

        if .keyIndexVar, .keyIndexVar.declareInScope
        
        if .intIndexVar, .intIndexVar.declareInScope

#### method evaluateAssignments() 

ForEachInArray:
If no valueVar.type, guess type from iterable's itemType

        if no .valueVar.nameDecl.findOwnMember('**proto**')
            var iterableType:Names.Declaration = .iterable.getResultType()          
            if iterableType and iterableType.findOwnMember('**item type**')  into var itemType
                .valueVar.nameDecl.setMember('**proto**',itemType)

#### method validatePropertyAccess() 
ForEachInArray: check if the iterable has a .length property.

        if .isMap, return

        var iterableType:Names.Declaration = .iterable.getResultType()

        if no iterableType 
            #.sayErr "ForEachInArray: no type declared for: '#{.iterable}'"
            do nothing

        else if no iterableType.findMember('length')
            .sayErr "ForEachInArray: no .length property declared in '#{.iterable}' type:'#{iterableType.toString()}'"
            logger.error iterableType.originalDeclarationPosition()

### Append to class Grammar.ForIndexNumeric ###

#### method declare()

        .keyIndexVar.declareInScope


### Append to class Grammar.ExceptionBlock
`ExceptionBlock: (exception|catch) catchVar-IDENTIFIER Body [finally Body]`

      method declare()

Exception blocks have a scope

        .createScope
        .addToScope .catchVar,{type:globalPrototype('Error')}


### Append to class Grammar.VariableRef ### Helper methods

`VariableRef: ['--'|'++']Identifier[Accessors]['--'|'++']`

`VariableRef` is a Variable Reference. 

#### method validatePropertyAccess() 

        if .parent is instance of Grammar.DeclareStatement
            declare valid .parent.specifier
            if .parent.specifier is 'valid'
                  return #declare valid xx.xx.xx

Start with main variable name, to check property names

        var actualVar = .tryGetFromScope(.name, {
                                informError:true
                                isForward:true
                                isDummy:true
                                nodeClass: Grammar.VariableDecl
                          })

now follow each accessor

        if no actualVar or no .accessors, return 

        for each ac in .accessors
            declare valid ac.name

for PropertyAccess, check if the property name is valid 

            if ac instanceof Grammar.PropertyAccess
                actualVar = .tryGetMember(actualVar, ac.name, {
                                informError:true
                                isDummy:true
                                nodeClass: Grammar.VariableDecl
                          })

else, for IndexAccess, the varRef type is now 'itemType'
and next property access should be on defined members of the type

            else if ac instanceof Grammar.IndexAccess
                actualVar = actualVar.findMember('**item type**')

else, for FunctionAccess, the varRef type is now function's return type'
and next property access should be on defined members of the return type

            else if ac instanceof Grammar.FunctionAccess
                declare ac:Grammar.FunctionAccess

                if no actualVar.findMember('call') and no actualVar.findMember('prototype')
                //if actualVar.findOwnMember('**proto**') into var prt
                //    if prt.name is 'prototype', prt=prt.parent
                //    if prt.name isnt 'Function'
                        //.warn "function call. '#{actualVar}' is class '#{prt.name}', not 'Function'"
                        .warn "function call. #{actualVar.info()} has no method 'call' nor 'prototype', it is not type:Function or Class"

Validate arguments against function parameters declaration

                if actualVar.nodeDeclared instanceof Grammar.FunctionDeclaration
                    or actualVar.nodeDeclared.constructor is Grammar.ClassDeclaration
                        ac.composeArgumentsList actualVar 

                actualVar = actualVar.findMember('**return type**')

if actualVar is a VarRef, find type in scope

            if actualVar instanceof Grammar.VariableRef
                .sayErr  "actualVar instanceof Grammar.VariableRef: #{actualVar.toString()}"
                declare actualVar:Grammar.VariableRef
                actualVar = actualVar.tryGetReference({
                                informError:true
                                isForward:true
                                isDummy:true
                                nodeClass: Grammar.VariableDecl
                          })

            if no actualVar, break

        end for #each accessor

        return actualVar


#### helper method tryGetReference(options:Names.DeclarationOptions) returns Names.Declaration

evaluate this VariableRef. 
Try to determine referenced NameDecl.
if we can reach a reference, return reference.
For classes, return ClassDeclaration.nameDecl (not ClassDeclaration.nameDecl.prototype)

        default options= new Names.DeclarationOptions

Start with main variable name

        var actualVar = .tryGetFromScope(.name, options)
        if no actualVar, return

now check each accessor
        
        if no .accessors, return actualVar

        var partial = .name

        for each ac in .accessors
            declare valid ac.name

for PropertyAccess

            if ac instanceof Grammar.PropertyAccess
                actualVar = .tryGetMember(actualVar, ac.name, options)

else, for IndexAccess, the varRef type is now 'itemType'
and next property access should be on defined members of the type

            else if ac instanceof Grammar.IndexAccess
                actualVar = .tryGetMember(actualVar, '**item type**')

else, for FunctionAccess, the varRef type is now function's return type'
and next property access should be on defined members of the return type

            else if ac instanceof Grammar.FunctionAccess
                actualVar = .tryGetMember(actualVar, '**return type**')

check if we can continue on the chain

            if actualVar isnt instance of Names.Declaration
              actualVar = undefined
              break
            else
              partial += ac.toString()

        end for #each accessor

        if no actualVar and options.informError
            .sayErr "'#{this}'. Reference can not be analyzed further than '#{partial}'"

        return actualVar

#### Helper Method getResultType() returns Names.Declaration
      
      return .tryGetReference()


### Append to class Grammar.FunctionAccess

##### helper method composeArgumentsList(actualVar:Names.Declaration) returns array

        var argsLength = .args? .args.length else 0

        var actualArgs=new Array(argsLength)
        var argumentSet=new Array(argsLength)
        var isClass: boolean
        var funcDecl: Grammar.FunctionDeclaration 
        var fnParams: Grammar.FunctionParameters

        var typeAny: Names.Declaration = .findInScope('any');

locate the FunctionDeclaration for the function we're calling

        if actualVar and actualVar.nodeDeclared

            if actualVar.nodeDeclared instanceof Grammar.FunctionDeclaration
                funcDecl = actualVar.nodeDeclared

if we're calling the constructor of a class, 
look for constructor declaration in the hierarchy
or assume default constructor. 
default constructor accepts a instance literal as construction parameter.

            else if actualVar.nodeDeclared.constructor is Grammar.ClassDeclaration

                isClass = true
                var classNameDecl = actualVar, classDecl: Grammar.ClassDeclaration
                // search upward in the hierarchy looking for a declared constructor
                do while classNameDecl.nodeDeclared into classDecl
                    if classDecl.constructorDeclaration into funcDecl, break //has a explicit constructor, got it
                    if no classDecl.varRefSuper, break //no super => super is Object => no explicit constructor
                    classNameDecl = classDecl.varRefSuper.tryGetReference({informError:true}) //jump to super name declaration
                loop

If we can't locate function declaration, we assume variadic and all types matching / default constructor

        if no funcDecl 

            if argsLength is 0
                return actualArgs // a call with no arguments

            if isClass //no funcDecl & isClass => calling a *default* constructor

                var firstArg = .args[0]

                if argsLength > 1
                    or firstArg.expression.operandCount isnt 1
                    or firstArg.expression.root.name isnt instance of Grammar.ObjectLiteral
                        .sayErr "class [#{actualVar}] default constructor accepts only one argument: a instance literal"

                // compose a call with _fastNew - single arg
                actualArgs[0] = firstArg.calcParam(actualVar,0)
                return actualArgs

            //calling an unknown function
            for each inx,arg in .args
                if arg.name, .sayErr "cannot determine function being called => cannot use named arguments"
                actualArgs[inx] = arg.calcParam(typeAny,0)

            return actualArgs

        end if no funcDecl

Here we have located a funcDecl, we:
- check parameter count / variadic fn
- reorder named arguments if present
- call _fastNew on instance literals

        var funcParamsLength = funcDecl.paramsDeclarations.list.length
        var paramPosition, paramNamesSet=[], namedParamOptionUsed:boolean

        if argsLength

            for each inx,arg in .args

                paramPosition = inx
                if arg.name //named argument: e.g: `server.listen(port=30371)` 
                    namedParamOptionUsed = true
                    var found
                    for each paramInx,param in funcDecl.paramsDeclarations.list
                        if param.name is arg.name 
                            found = true
                            paramPosition = paramInx
                            if param.name in paramNamesSet, .sayErr "duplicated parameter '#{param.name}'"
                            paramNamesSet.push param.name
                            break
                    end for
                    if not found
                        .sayErr "invalid parameter name: '#{arg.name}'"
                        funcDecl.sayErr "function declaration is here"
                else 
                    //positional argument
                    if namedParamOptionUsed, .sayErr "cannot include positional arguments after named arguments"
                    if paramPosition < funcParamsLength
                        paramNamesSet.push funcDecl.paramsDeclarations.list[paramPosition].name

                //try to get param declared type
                var paramType:Names.Declaration

                if paramPosition >= funcParamsLength
                    // past the declared parameters
                    if no funcDecl.paramsDeclarations.variadic
                        .sayErr "function accepts #{funcParamsLength? 'only #{funcParamsLength}':'no'} arguments."
                        funcDecl.sayErr "function declaration is here. (add '...' to declare as variadic)"
                        return actualArgs

                    paramType = typeAny

                else
                    //get param declared type
                    paramType = funcDecl.paramsDeclarations.list[paramPosition].nameDecl.findOwnMember('**proto**')
                    if paramType
                        if paramType.name is 'prototype' 
                            isClass = true
                            paramType = paramType.parent // class
                    else
                        paramType = typeAny 

                end if

                //set argument
                actualArgs[paramPosition] = arg.calcParam(paramType,inx,funcDecl)
                argumentSet[paramPosition] = true

            end for each arg

        end if arguments

make sure all the required parameters are set

        for each inx,param in funcDecl.paramsDeclarations.list
            if param.required and param.name not in paramNamesSet
                .sayErr "parameter ##{inx+1},'#{param.name}' is required"
                funcDecl.sayErr "function declaration is here"
        end for    

return array with arguements

        return actualArgs


### Append to class Grammar.FunctionArgument

##### helper method calcParam(typeNameDecl:Names.Declaration,inx,funcDecl:Grammar.FunctionDeclaration) returns array
inx and funcDecl are included to enhance error reporting 

        var expr = .expression 

if we're producing C, a instance literal {...} as parameter
gets converted to a _fastNew() call, initializing the instance with the provided values.

We're supporting a common practice in JS: pass a LiteralObject in place of a class

        if project.options.target is 'c'

            if expr.operandCount is 1 and expr.root.name instanceof Grammar.ObjectLiteral

                //Here we have a ObjectLiteral argument
                var objLit:Grammar.ObjectLiteral = expr.root.name

                if typeNameDecl.name isnt 'any' and no typeNameDecl.findOwnMember('prototype') 
                    .sayErr "Argument ##{inx+1}: passing a instance literal {...}, but the function expects: #{typeNameDecl}"
                    if funcDecl, funcDecl.sayErr "function declaration is here. To be able to pass a instance literal, define parameter type as a class"
                
                else
                    return objLit.calcFastNew(typeNameDecl.getComposedName())

else, just return argument expression

        return [expr]


### Append to class Grammar.AssignmentStatement ###

#### method evaluateAssignments() ## Grammar.AssignmentStatement 
    
check if we've got a a clear reference.

      var reference = .lvalue.tryGetReference()
      if reference isnt instanceof Names.Declaration, return 

if it is assigning string or number literal, force type 

      if .rvalue instanceof Grammar.Expression
          if .rvalue.root.name.constructor in [Grammar.StringLiteral,Grammar.NumberLiteral]
              var theLiteral = .rvalue.root.name
              // if it is assigning a literal, force type to string|number|array
              reference.setMember('**proto**', globalPrototype(theLiteral.type))
              return

      if reference.findOwnMember('**proto**'), return #has a type already

check if we've got a clear rvalue.
if we do, set type for lvalue (unless is "null" or "undefined", they destroy type info)

      reference.assignTypeFromValue .rvalue


/*
#### method declareByAssignment()

Here we check for lvalue VariableRef in the form:

`exports.x = xx`, `module.exports.x = xx` and `xx.prototype.yy =`

We consider this assignments as 'declarations' of members rather than variable references to check.

Start with main variable name

        var varRef = .lvalue

        var keywordFound

        if varRef.name is 'exports' #start with 'exports'
            keywordFound = varRef.name

        if no varRef.accessors 

          if keywordFound # is: `exports = x`, it does not work in node-js
              .sayErr "'exports = x', does not work. You need to do: 'module.exports = x'"

          return # no accessors to check

        var actualVar = .findInScope(varRef.name)
        if no actualVar, return

now check each accessor
        
        var createName 

        for each index,ac in varRef.accessors
            declare valid ac.name

for PropertyAccess

            if ac instanceof Grammar.PropertyAccess

              #if we're after 'exports|prototype' keyword and this is the last accessor,
              #then this is the name to create
              if keywordFound and index is varRef.accessors.length-1
                  createName = ac.name
                  break

check for 'exports' or 'prototype', after that, last accessor is property declaration

              if ac.name in ['exports','prototype']
                keywordFound = ac.name

              actualVar =  actualVar.findMember(ac.name)
              if no actualVar, break

else, if IndexAccess or function access, we exit analysis

            else 
              return #exit

        end for #each accessor in lvalue, look for module.exports=...

if we found 'exports' or 'prototype', and we reach a valid reference

        if keywordFound and actualVar 
          
            if createName # module.exports.x =... create a member
              actualVar = .addMemberTo(actualVar,createName) # create x on module.exports

            #try to execute assignment, so exported var points to content
            var content = .rvalue.getResultType() 
            if content instanceof Names.Declaration
                actualVar.makePointTo content
*/

### Append to class Grammar.Expression ###

#### Helper Method getResultType() returns Names.Declaration
Try to get return type from a simple Expression

        declare valid .root.getResultType:function
        return .root.getResultType() # .root is Grammar.Oper or Grammar.Operand


### Append to class Grammar.Oper ###

for 'into var x' oper, we declare the var, and we deduce type

#### Method declare() 
        
        if .intoVar is '*r' # is a into-assignment operator with 'var' declaration

            var varRef = .right.name
            if varRef isnt instance of Grammar.VariableRef
                .throwError "Expected 'variable name' after 'into var'"

            if varRef.accessors 
                .throwError "Expected 'simple variable name' after 'into var'"
            
            .addToScope .declareName(varRef.name,{
                                        type:varRef.type
                                        nodeClass:Grammar.VariableDecl
                                      })

#### method evaluateAssignments() 
    
for into-assignment operator

      if .name is 'into' # is a into-assignment operator

check if we've got a clear reference (into var x)

          if .right.name instance of Grammar.VariableRef

              declare valid .right.name.tryGetReference:function
              var nameDecl = .right.name.tryGetReference()

              if nameDecl isnt instanceof Names.Declaration, return 
              if nameDecl.findOwnMember('**proto**'), return #has a type already

check if we've got a clear .left (value to be assigned) type
if we do, set type for .rigth ('into var x') (unless is "null" or "undefined", they destroy type info)

              nameDecl.assignTypeFromValue .left


#### Helper Method getResultType() returns Names.Declaration
Try to get return type from this Oper (only for 'new' unary oper)

        declare valid .right.getResultType

        if .name is 'new'
            return .right.getResultType() #.right is Grammar.Operand


### Append to class Grammar.Operand ###

#### Helper Method getResultType() returns Names.Declaration
Try to get return type from this Operand

        declare valid .name.type
        declare valid .name.getResultType
        declare valid .name.tryGetReference

        if .name instance of Grammar.ObjectLiteral
            return .name.getResultType()

        else if .name instance of Grammar.Literal
            return globalPrototype(.name.type)

        else if .name instance of Grammar.VariableRef
            return .name.tryGetReference()

        else if .name instance of Grammar.FunctionDeclaration
            return globalPrototype('Function')

### Append to class Grammar.DeclareStatement
#### method declare() # pass 1, declare as props

declare [all] x:type
declare [global] var x
declare on x
declare valid x.y.z
      

      if .specifier is 'on'

          var reference = .tryGetFromScope(.name,{isForward:true})

          if String.isCapitalized(reference.name) //let's assume is a Class
              if no reference.findOwnMember('prototype'), reference.addMember('prototype')
              reference=reference.findOwnMember('prototype')

          for each varDecl in .names
              .addMemberTo reference, varDecl.createNameDeclaration()

else: declare (name affinity|var) (VariableDecl,)

      else if .specifier in ['affinity','var']

          for each varDecl in .names

            varDecl.nameDecl = varDecl.createNameDeclaration()

            if .specifier is 'var'
                if .globVar
                    project.rootModule.addToScope varDecl.nameDecl
                else
                    .addToScope varDecl.nameDecl

            else if .specifier is 'affinity'
                var classDecl = .getParent(Grammar.ClassDeclaration)
                if no classDecl
                    .sayErr "'declare name affinity' must be included in a class declaration"
                    return
                #add as member to nameAffinity, referencing class decl (.nodeDeclared)
                varDecl.nameDecl.nodeDeclared = classDecl
                declare varDecl.name:string
                nameAffinity.members.set varDecl.name.capitalized(), classDecl.nameDecl

if .specifier is 'on-the-fly', the type will be converted on next passes over the created Names.Declaration.
On the method validatePropertyAccess(), types will be switched "on the fly" 
while checking property access.

#### method evaluateAssignments() # Grammar.DeclareStatement ###
Assign specific type to varRef - for the entire compilation

      if .specifier is 'type'
          if .varRef.tryGetReference({informError:true}) into var actualVar
              .setTypes actualVar

#### helper method setTypes(actualVar:Names.Declaration) # Grammar.DeclareStatement ###
Assign types if it was declared

      #create type on the fly, overwrite existing type

      if .type
        .setSubType actualVar,.type.mainType,'**proto**'
        .setSubType actualVar,.type.itemType,'**item type**'

#### helper method setSubType(actualVar:Names.Declaration, toSet, propName ) 
Assign type if it was declared

      if toSet #create type on the fly
          //var act=actualVar.findMember(propName)
          //print "set *type* was #{act} set to #{toSet}"
          actualVar.setMember propName, toSet
          var result = actualVar.processConvertTypes()
          if result.failures, .sayErr "can't find type '#{toSet}' in scope"

#### method validatePropertyAccess() # Grammar.DeclareStatement ###

declare members on the fly, with optional type

      var actualVar:Names.Declaration

      case .specifier 

        when 'valid':

            actualVar = .tryGetFromScope(.varRef.name,{informError:true})
            if no actualVar, return

            for each ac in .varRef.accessors
                declare valid ac.name

                if ac isnt instance of Grammar.PropertyAccess 
                    actualVar = undefined
                    break
                
                if ac.name is 'prototype'
                    actualVar = actualVar.findOwnMember(ac.name) or .addMemberTo(actualVar, ac.name)  
                else
                    actualVar = actualVar.findMember(ac.name) or .addMemberTo(actualVar, ac.name)

            end for

            if actualVar, .setTypes actualVar

        when 'on-the-fly':
            #set type on-the-fly, from here until next type-assignment
            #we allow more than one "declare x:type" on the same block
            if .varRef.tryGetReference({informError:true}) into actualVar
                .setTypes actualVar


### helper function AddGlobalClasses(...)
  
        var nameDecl

        for each name in arguments.toArray()

            nameDecl = globalScope.addMember(name,{nodeClass:Grammar.ClassDeclaration})

            var prtypeNameDecl = nameDecl.addMember('prototype',{nodeClass:Grammar.VariableDecl})

            nameDecl.setMember '**return type**',prtypeNameDecl

            // add to name affinity
            if not nameAffinity.members.has(name)
                nameAffinity.members.set name, nameDecl

----------------------------
### Append to class Names.Declaration ###

#### helper method calcComposedPrefix returns Array

if this node is member of a namespace, goes up the parent chain
composing the prefix. e.g. a property x in module Foo, namespace Bar => `Foo_Bar_`

        var result = []

        if .nodeDeclared instanceof Grammar.ImportStatementItem
            // imported modules create a local var, but act as global var
            //since all others import of the same name, return the same content 
            // no prefix for "import" module declared vars
            return result

        var lastWasExported = .isExported

        var node = this.parent

        while node 

            if node.isScope and node.nodeDeclared isnt instance of Grammar.NamespaceDeclaration
                break // end composing when a scope is reached. Except for NamespaceDeclaration
                // NamespaceDeclaration scope exists only to be able to use namespace items
                // without namespace prefix inside the namespace declaration, 
                // but it is not a real scope at execution time

            if node.name isnt 'prototype' //skip .prototype. in C
                result.unshift node.name
                lastWasExported = node.isExported

            node = node.parent

        end while loop

if we reached module scope, (and not Global Scope) 
then it's a var|fn|class declared at module scope.
If the object is 'public|exported' we add module.fileinfo.base to the name 
(to use always it's "global" name)

        if lastWasExported and node and node.isScope and node.nodeDeclared.constructor is Grammar.Module 
            var scopeModule = node.nodeDeclared
            if scopeModule.name isnt '*Global Scope*' //except for global scope
                  result.unshift scopeModule.fileInfo.base


        return result


#### helper method getComposedPrefix() returns string

        return .calcComposedPrefix().join("_")

#### helper method getComposedName() returns string

        var result = .calcComposedPrefix()

        if .name isnt 'prototype' //skip .prototype. in C
            result.push .name

        return result.join("_")
