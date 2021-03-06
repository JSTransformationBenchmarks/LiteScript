Producer JS
===========

The `producer` module extends Grammar classes, adding a `produce()` method 
to generate target code for the node.

The comp1iler calls the `.produce()` method of the root 'Module' node 
in order to return the compiled code for the entire tree.

We extend the Grammar classes, so this module require the `Grammar` module.

    var Grammar = require('./Grammar')


Identifier aliases
------------------

This are a few aliases to most used built-in identifiers:

    var IDENTIFIER_ALIASES =
      'on':         'true'
      'off':        'false'
      'me':         'this'


Utility 
-------

    var NL = '\n' # New Line constant
    var util = require('./util')

Operator Mapping
================

Many LiteScript operators can be easily mapped one-to-one with their JavaScript equivalents.

    var OPER_TRANSLATION =

      'no':           '!'
      'not':          '!'
      'unary -':      '-'
      'unary +':      '+'

      'type of':      'typeof'
      'instance of':  'instanceof'

      'is':           '==='
      'isnt':         '!=='
      '<>':           '!=='
      'and':          '&&'
      'but':          '&&'
      'or':           '||'
      'has property': 'in'

    function operTranslate(name:string)
      return name.translate(OPER_TRANSLATION)

---------------------------------
---------------------------------

### Append to class Grammar.ASTBase

Helper methods and properties, valid for all nodes

#### properties skipSemiColon 

#### helper method compilerVar(name) 

helper function compilerVar(name)
return root.compilerVars.members[name].value

        var asked = me.getRootNode().compilerVars.findOwnMember(name) 
        if asked 
          declare valid asked.value
          return asked.value


#### helper method outPrevLinesComments() 

outPrevLinesComments helper method: output comments from previous lines
before the statement

      declare valid me.lexer.lastOutCommentLine
      declare valid me.lexer.LineTypes.CODE 
      declare valid me.lexer.infoLines

      var inx = me.lineInx
      if inx<1 
        return

      if not me.lexer.lastOutCommentLine
        me.lexer.lastOutCommentLine = -1

find comment lines in the previous lines of code. 

      var preInx = inx
      while preInx and preInx>me.lexer.lastOutCommentLine 
        preInx-=1
        if me.lexer.infoLines[preInx].type is me.lexer.LineTypes.CODE 
          preInx+=1
          break

Output prev comments lines (also blank lines)

      me.outLinesAsComment preInx, inx-1

    #end method


#### helper method getEOLComment() 
getEOLComment: get the comment at the end of the line

Check for "postfix" comments. These are comments that occur at the end of the line,
such as `a = 1 #comment`. We want to try to add these at the end of the current JavaScript line.

        declare valid me.lexer.lastOutCommentLine
        declare valid me.lexer.LineTypes.CODE 
        declare valid me.lexer.infoLines
        
        var inx = me.lineInx
        var infoLine = me.lexer.infoLines[inx]

        #declare on infoLine
        #    indent,text,tokens:array      

        if infoLine.tokens and infoLine.tokens.length
            var lastToken = infoLine.tokens[infoLine.tokens.length-1]
            if lastToken.type is 'COMMENT'
                return "#{lastToken.value.startsWith('//')?'':'//'} #lastToken.value"


#### helper method assignIfUndefined(name,value) 
          
          declare valid value.root.name.name
          #do nothing if value is 'undefined'
          if value.root.name.name is 'undefined' #Expression->Operand->VariableRef->name
            me.out "// ",name,": undefined",NL
            return

          me.out "if(",name,'===undefined) ',name,"=",value,";",NL


--------------------------------
--------------------------------

JavaScript Producer Functions
==============================

### append to class Grammar.Body ###

A "Body" is an ordered list of statements.

"Body"s lines have all the same indent, representing a scope.

"Body"s are used for example, to parse an `if` statement body and `else` body, `for` loops, etc.

      method produce()

        me.outCode.startNewLine()
        for statement in me.statements
          statement.produce()

        me.out NL


### append to class Grammar.Module ###
Same as Body

      method produce() 
        Grammar.Body.prototype.produce.apply(this,arguments)


-------------------------------------
### append to class Grammar.Statement ###

`Statement` objects call their specific statement node's `produce()` method
after adding any comment lines preceding the statement

      method produce()

        declare valid me.lexer.lastOriginalCodeComment
        declare valid me.lexer.LineTypes.CODE 
        declare valid me.lexer.infoLines
        declare valid me.statement.body
        declare valid me.statement.skipSemiColon

add comment lines, in the same position as the source

        me.outPrevLinesComments()

To ease reading of compiled code, add original Lite line as comment 
(except for EndStatement ant others which produdce it)

        if me.lexer.lastOriginalCodeComment<me.lineInx
          if not (me.statement.constructor in [
              Grammar.ClassDeclaration,Grammar.VarStatement,Grammar.CompilerStatement
              Grammar.DeclareStatement,Grammar.AssignmentStatement
              Grammar.PropertiesDeclaration 
            ])
            me.out {COMMENT: me.lexer.infoLines[me.lineInx].text.trim()},NL
        me.lexer.lastOriginalCodeComment = me.lineInx

call the specific statement (if,for,print,if,function,class,etc) .produce()

        me.out me.statement

add ";" after the statement
then EOL comment (if it isnt a multiline statement)
then NEWLINE

        if not me.statement.skipSemiColon
          me.out ";"
          if not me.statement.body
            me.out me.getEOLComment()


---------------------------------
### append to class Grammar.ThrowStatement ###

      method produce()
          if me.specifier is 'fail'
            me.out "throw new Error(", me.expr,")"
          else
            me.out "throw ", me.expr


### Append to class Grammar.ReturnStatement ###

      method produce()
        me.out "return"
        if me.expr
          me.out " ",me.expr


### Append to class Grammar.FunctionCall ###

      method produce() 

        me.varRef.produce()
        if me.varRef.executes, return #if varRef already executes, nothing more to do
        me.out "()" #add (), so JS executes de function call


### append to class Grammar.Operand ###

`Operand:
  |NumberLiteral|StringLiteral|RegExpLiteral
  |ParenExpression|ArrayLiteral|ObjectLiteral|FunctionDeclaration
  |VariableRef

A `Operand` is the left or right part of a binary oper
or the only Operand of a unary oper.

      method produce()

        me.out me.name, me.accessors

      #end Operand


### append to class Grammar.UnaryOper ###

`UnaryOper: ('-'|new|type of|not|no|bitwise not) `

A Unary Oper is an operator acting on a single operand.
Unary Oper inherits from Oper, so both are `instance of Oper`

Examples:
1) `not`     *boolean negation*     `if not a is b`
2) `-`       *numeric unary minus*  `-(4+3)`
3) `new`     *instantiation*        `x = new classNumber[2]`
4) `type of` *type name access*     `type of x is classNumber[2]` 
5) `no`      *'falsey' check*       `if no options then options={}` 
6) `~`       *bit-unary-negation*   `a = ~xC0 + 5`

      method produce() 
        
        var translated = operTranslate(me.name)

if it is "boolean not", add parentheses, because js has a different precedence for "boolean not"
-(prettier generated code) do not add () for simple "falsey" variable check

        if translated is "!" and not (me.name is "no" and me.right.name instanceof Grammar.VariableRef)
          var prepend ="("
          var append=")"

add a space if the unary operator is a word. Example `typeof`

        if /\w/.test(translated) 
          translated+=" "

        me.out translated, prepend, me.right, append


### append to class Grammar.Oper ###

      method produce()

        var oper = me.name

default mechanism to handle 'negated' operand

        var prepend,append
        if me.negated # NEGATED

if NEGATED and the oper is `is` we convert it to 'isnt'.
'isnt' will be translated to !==

            if oper is 'is' # Negated is ---> !==
              oper = '!=='

else -if NEGATED- we add `!( )` to the expression

            else 
              prepend ="!("
              append=")"

Check for special cases: 

1) 'in' operator, requires swapping left and right operands and to use `.indexOf(...)>=0`
example: `x in [1,2,3]` -> `[1,2,3].indexOf(x)>=0`
example: `x not in [1,2,3]` -> `[1,2,3].indexOf(x)==-1`
example: `char not in myString` -> `myString.indexOf(char)==-1`
example (`arguments` pseudo-array): `'lite' not in arguments` -> `Array.prototype.slice.call(arguments).indexOf(char)==-1`

        if me.name is 'in'
            me.out me.right,".indexOf(",me.left,")", me.negated? "===-1":">=0"

fix when used on JS built-in array-like `arguments`

            me.outCode.currLine = me.outCode.currLine.replace(/\barguments.indexOf\(/,'Array.prototype.slice.call(arguments).indexOf(')

2) *'has property'* operator, requires swapping left and right operands and to use js: `in`

        else if me.name is 'has property'
            me.out prepend, me.right," in ",me.left, append

else we have a direct translatable operator. 
We out: left,operator,right

        else
            me.out prepend, me.left, ' ', operTranslate(oper), ' ', me.right , append

        #end if


### append to class Grammar.Expression ###

      method produce(options) 

        declare on options
          negated

Produce the expression body, negated if options={negated:true}

        default options=
          negated: undefined

        var prepend=""
        var append=""
        if options.negated

(prettier generated code) Try to avoid unnecessary parens after '!' 
for example: if the expression is a single variable, as in the 'falsey' check: 
Example: `if no options.log then... ` --> `if (!options.log) {...` 
we don't want: `if (!(options.log)) {...` 

          if me.operandCount is 1 
              #no parens needed
              prepend = "!"
          else
              prepend = "!("
              append = ")"
          #end if
        #end if negated

produce the expression body

        me.out prepend, me.root, append


### append to class Grammar.VariableRef ###

`VariableRef: ['--'|'++'] IDENTIFIER [Accessors] ['--'|'++']`

`VariableRef` is a Variable Reference. 

 a VariableRef can include chained 'Accessors', which can:
 *access a property of the object : `.`-> PropertyAccess `[`->IndexAccess
 *assume the variable is a function and perform a function call :  `(`-> FunctionAccess

      method produce() 

Prefix ++/--, varName, Accessors and postfix ++/--

        me.out me.preIncDec, me.name.translate(IDENTIFIER_ALIASES), me.accessors, me.postIncDec


### append to class Grammar.AssignmentStatement ###

      method produce() 

        me.out me.lvalue, ' ', operTranslate(me.name), ' ', me.rvalue


-------
### append to class Grammar.DefaultAssignment ###

      method produce() 

        me.process(me.assignment.lvalue, me.assignment.rvalue)

        me.skipSemiColon = true

#### helper Functions

      #recursive duet 1
      helper method process(name,value)

if it is ObjectLiteral: recurse levels, else, a simple 'if undefined, assignment'

check if it's a ObjectLiteral (level indent)

          if value instanceof Grammar.ObjectLiteral
            me.processItems name, value # recurse Grammar.ObjectLiteral

else, simple value (Expression)

          else
            me.assignIfUndefined name, value # Expression


      #recursive duet 2
      helper method processItems(main, objectLiteral) 

          me.out "if(!",main,') ',main,"={};",NL

          for nameValue in objectLiteral.items
            var itemFullName = [main,'.',nameValue.name]
            me.process(itemFullName, nameValue.value)


    #end helper recursive functions


-----------
## Accessors
We just defer to JavaScript's built in `.` `[ ]` and `( )` accessors

### append to class Grammar.PropertyAccess ##
      method produce() 
        me.out ".",me.name

### append to class Grammar.IndexAccess
      method produce() 
        me.out "[",me.name,"]"

### append to class Grammar.FunctionAccess
      method produce() 
        me.out "(",{CSL:me.args},")"

-----------

### Append to class Grammar.ASTBase
#### helper method lastLineInxOf(list:Grammar.ASTBase array) 

More Helper methods, get max line of list

        var lastLine = me.lineInx
        for item in list
            if item.lineInx>lastLine 
              lastLine = item.lineInx

        return lastLine


#### method getOwnerPrefix() returns array

check if we're inside a ClassDeclaration or AppendToDeclaration.
return prefix for item to be appended

        var parent = me.getParent(Grammar.ClassDeclaration)

        if no parent, return 
    
        var ownerName, toPrototype
        if parent instance of Grammar.AppendToDeclaration
          #append to class prototype or object
          declare parent:Grammar.AppendToDeclaration
          toPrototype = parent.optClass
          ownerName = parent.varRef
        
        else # in a ClassDeclaration
          declare valid me.toNamespace
          toPrototype = not me.toNamespace #if it's a "namespace properties" or "namespace method"
          ownerName = parent.name

        return [ownerName, toPrototype? ".prototype." : "." ]


---
### Append to class Grammar.PropertiesDeclaration ###

'var' followed by a list of comma separated: var names and optional assignment

      method produce() 

        me.outLinesAsComment me.lineInx, me.lastLineInxOf(me.list)

        var prefix = me.getOwnerPrefix()

        for varDecl in me.list
          if varDecl.assignedValue
            me.out prefix, varDecl.name,"=",varDecl.assignedValue,";",NL

        me.skipSemiColon = true

### Append to class Grammar.VarStatement ###

'var' followed by a list of comma separated: var names and optional assignment

      method produce() 

        declare valid me.keyword
        declare valid me.compilerVar
        declare valid me.public

        if me.keyword is 'let' and me.compilerVar('ES6')
          me.out 'let '

        else
          me.out 'var '

Now, after 'var' or 'let' out one or more comma separated VariableDecl 
  
        me.out {CSL:me.list}

If 'var' was adjectivated 'public', add to module.exports

        if me.public
          for item in me.list
            declare valid item.name
            me.out ";", NL,'exports.',item.name,' = ', item.name,NL


### Append to class Grammar.VariableDecl ###

variable name and optionally assign a value

      method produce() 

It's a `var` keyword or we're declaring function parameters.
In any case starts with the variable name
      
          me.out me.name

          declare valid me.keyword
          declare valid me.public

If this VariableDecl is from a 'var' statement, we force assignment (to avoid subtle bugs,
in LiteScript, 'var' declaration assigns 'undefined')

          if me.parent instanceof Grammar.VarStatement 
              me.out ' = ',me.assignedValue or 'undefined'

else, if this VariableDecl is from function parameters, 
if it has AssginedValue, we out assignment if ES6 is available. 
(ES6 implements 'default' for parameters, so `function myFunc(a=3)` is valid in ES6)

          else
            if me.assignedValue and me.compilerVar('ES6')
                me.out ' = ',me.assignedValue

    #end VariableDecl


### Append to class Grammar.SingleLineStatement ###

      method produce()
    
        var bare=[]
        for statement in me.statements
            bare.push statement.statement
        me.out NL,"    ",{CSL:bare, separator:";"}


### Append to class Grammar.IfStatement ###

      method produce() 

        declare valid me.elseStatement.produce

        if me.body instanceof Grammar.SingleLineStatement
            me.out "if (", me.conditional,") {",me.body, "}"
        else
            me.out "if (", me.conditional, ") {", me.getEOLComment()
            me.out  me.body, "}"

        if me.elseStatement
          me.elseStatement.produce()


### Append to class Grammar.ElseIfStatement ###

      method produce() 

        me.out NL,"else ", me.nextIf

### Append to class Grammar.ElseStatement ###

      method produce()

        me.out NL,"else {", me.body, "}"

### Append to class Grammar.ForStatement ###

There are 3 variants of `ForStatement` in LiteScript

      method produce() 

        declare valid me.variant.iterable
        declare valid me.variant.produce

Pre-For code. If required, store the iterable in a temp var.
(prettier generated code) Only if the iterable is a complex expression, 
(if it can have side-effects) we store it in a temp 
var in order to avoid calling it twice. Else, we use it as is.

        var iterable = me.variant.iterable

        declare iterable:Grammar.Expression
        declare valid iterable.root.name.hasSideEffects

        if iterable 
          if iterable.operandCount>1 or iterable.root.name.hasSideEffects or iterable.root instanceof Grammar.Literal
            iterable = me.outCode.getUniqueVarName('list')  #unique temp iterable var name
            me.out "var ",iterable,"=",me.variant.iterable,";",NL

        me.variant.produce(iterable)

Since al 3 cases are closed with '}; //comment', we skip statement semicolon

        me.skipSemiColon = true


### Append to class Grammar.ForEachProperty
### Variant 1) 'for each property' to loop over *object property names* 

`ForEachProperty: for each [own] property name-VariableDecl in object-VariableRef`

      method produce(iterable) 

          me.out "for ( var ", me.indexVar, " in ", iterable, ") "
          if me.ownOnly 
            me.out "if (",iterable,".hasOwnProperty(",me.indexVar,")) "

          me.out "{", me.body, "}; // end for each property",NL


### Append to class Grammar.ForIndexNumeric
### Variant 2) 'for index=...' to create *numeric loops* 

`ForIndexNumeric: for index-VariableDecl "=" start-Expression [,|;] (while|until) condition-Expression [(,|;) increment-Statement]`

Examples: `for n=0 while n<10`, `for n=0 to 9`
Handle by using a js/C standard for(;;){} loop

      method produce()

        declare valid me.endExpression.produce

        me.out "for( var ",me.indexVar, "=", me.startIndex, "; "

        if me.conditionPrefix is 'to'
            #'for n=0 to 10' -> for(n=0;n<=10;...
            me.out me.indexVar,"<=",me.endExpression

        else # is while|until

produce the condition, negated if the prefix is 'until'

          #for n=0, while n<arr.length  -> for(n=0;n<arr.length;...
          #for n=0, until n >= arr.length  -> for(n=0;!(n>=arr.length);...
          me.endExpression.produce( {negated: me.conditionPrefix is 'until' })

        me.out "; "

if no increment specified, the default is indexVar++

        if me.increment
          me.out me.increment
        else 
          me.out me.indexVar,"++"

        me.out "){", me.body, "}; // end for ", me.indexVar,NL


### Append to class Grammar.ForEachInArray
### Variant 3) 'for each index' to loop over *Array indexes and items*

`ForEachInArray: for each [index-VariableDecl,]item-VariableDecl in array-VariableRef`

      method produce(iterable)

Create a default index var name if none was provided

        var indexVar = me.indexVar
        if no indexVar
          indexVar = me.mainVar+'__inx' #default index var name

        me.out "for ( var ", indexVar,"=0,",me.mainVar,"=undefined; ",indexVar,"<",iterable,".length; ",indexVar,"++) {",NL
        
        me.body.out "var ",me.mainVar,"=",iterable,"[",indexVar,"];"
        
        me.out me.body, "}; // end for each in ", me.iterable,NL


### Append to class Grammar.WhileUntilExpression ###

      method produce(options) 

If the parent ask for a 'while' condition, but this is a 'until' condition,
or the parent ask for a 'until' condition and this is 'while', we must *negate* the condition.

        declare valid me.expr.produce

        default options = 
          askFor: undefined
          negated: undefined

        if options.askFor and me.name isnt options.askFor
            options.negated = true

*options.askFor* is used when the source code was, for example,
`do until Expression` and we need to code: `while(!(Expression))` 
or the code was `loop while Expression` and we need to code: `if (!(Expression)) break` 

when you have a `until` condition, you need to negate the expression 
to produce a `while` condition. (`while NOT x` is equivalent to `until x`)

        me.expr.produce(options)


### Append to class Grammar.DoLoop ###

      method produce() 

Note: **WhileUntilLoop** symbol has **DoLoop** as *prototype*, so this *.produce()* method
is used by both symbols.

        if me.postWhileUntilExpression 

if we have a post-condition, for example: `do ... loop while x>0`, 

            me.out "do{", me.getEOLComment()
            me.out me.body
            me.out "} while ("
            me.postWhileUntilExpression.produce({askFor:'while'})
            me.out ");",{COMMENT:"end loop"},NL

else, optional pre-condition:
  
        else

            me.out 'while('
            if me.preWhileUntilExpression
              me.preWhileUntilExpression.produce({askFor:'while'})
            else 
              me.out 'true'
            me.out '){', me.body , "};",{COMMENT:"end loop"},NL

        end if


### Append to class Grammar.LoopControlStatement ###
This is a very simple produce() to allow us to use the `break` and `continue` keywords.
  
      method produce() 
        me.out me.control

### Append to class Grammar.DoNothingStatement ###

      method produce()
        me.out "null"

### Append to class Grammar.ParenExpression ###

A `ParenExpression` is just a normal expression surrounded by parentheses.

      method produce() 
        me.out "(",me.expr,")"

### Append to class Grammar.ArrayLiteral ###

A `ArrayLiteral` is a definition of a list like `[1, a, 2+3]`. We just pass this through to JavaScript.

      method produce() 
        me.out "[",{CSL:me.items},"]"

### Append to class Grammar.NameValuePair ###

A `NameValuePair` is a single item in an object definition. Since we copy js for this, we pass this straight through 

      method produce() 
        me.out me.name,": ",me.value

### Append to class Grammar.ObjectLiteral ###

A `ObjectLiteral` is an object definition using key/value pairs like `{a:1,b:2}`. 
JavaScript supports this syntax, so we just pass it through. 

      method produce()
        me.out "{",{CSL:me.items},"}"

### Append to class Grammar.FreeObjectLiteral ###

A `FreeObjectLiteral` is an object definition using key/value pairs, but in free-form
(one NameValuePair per line, indented, comma is optional)

      method produce()
        me.out "{",{CSL:me.items, freeForm:true},"}"


### Append to class Grammar.FunctionDeclaration ###

`FunctionDeclaration: '[public][generator] (function|method|constructor) [name] '(' FunctionParameterDecl* ')' Block`

`FunctionDeclaration`s are function definitions. 

`public` prefix causes the function to be included in `module.exports`
`generator` prefix marks a 'generator' function that can be paused by `yield` (js/ES6 function*)

     method produce()

      declare valid me.public
      declare valid me.generator 
      declare valid me.exceptionBlock

Generators are implemented in ES6 with the "function*" keyword (note the asterisk)

      var generatorMark = ""
      if me.generator and me.compilerVar('ES6')
        generatorMark="*"

check if we're inside a ClassDeclaration or AppendToDeclaration

      if me instance of Grammar.ConstructorDeclaration
          # class constructor: JS's function-class-object-constructor
          me.out "function ",me.getParent(Grammar.ClassDeclaration).name

get owner where this method belongs to

      else
          var prefix = me.getOwnerPrefix()

          if no prefix and me instance of Grammar.MethodDeclaration
              debugger
              fail with "method #me.name. Can not determine owner object"

          if no prefix #no class, just a scope function
              me.out "function ",me.name, generatorMark
          else
              me.out prefix, me.name," = function",generatorMark
now produce function parameters declaration
       
      me.out "(", {CSL:me.paramsDeclarations}, "){", me.getEOLComment()

if the function has a exception block, insert 'try{'

      for each statement in me.body.statements
        if statement.statement instance of Grammar.ExceptionBlock
            me.out " try{",NL
            break

if params defaults where included, we assign default values to arguments 
(if ES6 enabled, they were included abobve in ParamsDeclarations production )

      if me.paramsDeclarations and not me.compilerVar('ES6')
          for paramDecl in me.paramsDeclarations
            if paramDecl.assignedValue 
                me.body.assignIfUndefined paramDecl.name, paramDecl.assignedValue
          #end for
      #end if

now produce function body

      me.body.produce()

close the function

      me.out "}"

If the function was adjectivated 'public', add to module.exports

      if me.public
        me.out ";",NL,'exports.',me.name,'=',me.name


--------------------
### Append to class Grammar.PrintStatement ###
`print` is an alias for console.log

      method produce()
        me.out "console.log(",{"CSL":me.args},")"


--------------------
### Append to class Grammar.EndStatement ###

Marks the end of a block. It's just a comment for javascript

      method produce()

        declare valid me.lexer.lastOriginalCodeComment
        declare valid me.lexer.infoLines

        if me.lexer.lastOriginalCodeComment<me.lineInx
          me.out {COMMENT: me.lexer.infoLines[me.lineInx].text}
        
        me.skipSemiColon = true

--------------------
### Append to class Grammar.CompilerStatement ###

      method produce()

first, out as comment this line

        me.outLineAsComment me.lineInx

if it's a conditional compile, output body is option is Set

        if me.conditional
            if me.compilerVar(me.conditional)
                declare valid me.body.produce
                me.body.produce()

        me.skipSemiColon = true


--------------------
### Append to class Grammar.DeclareStatement ###

Out as comments

      method produce()

        me.outLinesAsComment me.lineInx, me.lastLineInxOf(me.names)
        me.skipSemiColon = true


----------------------------
### Append to class Grammar.ClassDeclaration ###

Classes contain a code block with properties and methods definitions.

      method produce()

        me.out {COMMENT:" Class "},me.name
        if me.varRefSuper
          me.out ", extends|inherits from ", me.varRefSuper
        me.out ", constructor:",NL
      
First, since in JS we have a object-class-function-constructor  all-in-one
we need to get the class constructor, and separate other class items.

        declare theConstructor:Grammar.FunctionDeclaration
        declare valid me.produce_AssignObjectProperties
        declare valid me.public

        var theConstructor = null
        var methodsAndProperties = []

        if me.body
          for item at index in me.body.statements

            if item.statement instanceof Grammar.ConstructorDeclaration 

              if theConstructor # what? more than one?
                me.throwError('Two constructors declared for class #{me.name}')

              theConstructor = item.statement

            else 
              methodsAndProperties.push item

        #end if body

        if theConstructor
          me.out theConstructor,";",NL
        else
          #out a default "constructor"
          me.out "function ",me.name,"(){"
          if me.varRefSuper
              me.out NL,"    // default constructor: call super.constructor"
              me.out NL,"    return ",me.varRefSuper,".prototype.constructor.apply(this,arguments)",NL
          me.out "};",NL
        #end if

Set parent class if we have one indicated

        if me.varRefSuper
          me.out '// ',me.name,' (extends|super is) ',me.varRefSuper, NL
          me.out me.name,'.prototype.__proto__ = ', me.varRefSuper,'.prototype;',NL 

now out class body, which means create properties in the object-function-class prototype

        me.out NL,'// declared properties & methods',NL
        me.out methodsAndProperties

If the class was adjectivated 'public', add to module.exports

        if me.public
          me.out NL,'exports.',me.name,' = ', me.name,";"

        me.out NL,{COMMENT:"end class "},me.name,NL
        me.skipSemiColon = true


### Append to class Grammar.AppendToDeclaration ###

Any class|object can have properties or methods appended at any time. 
Append-to body contains properties and methods definitions.

      method produce() 
        me.out me.body
        me.skipSemiColon = true


### Append to class Grammar.TryCatch ###

      method produce() 

        me.out "try{", me.body, me.exceptionBlock

### Append to class Grammar.ExceptionBlock ###

      method produce() 

        me.out NL,"}catch(",me.catchVar,"){", me.body, "}"

        if me.finallyBody
          me.out NL,"finally{", me.finallyBody, "}"


### Append to class Grammar.WaitForAsyncCall ###

      method produce()

        declare valid me.call.funcRef
        declare valid me.call.args

        me.out "wait.for(", {CSL:[me.call.funcRef].concat(me.call.args)} ,")"


