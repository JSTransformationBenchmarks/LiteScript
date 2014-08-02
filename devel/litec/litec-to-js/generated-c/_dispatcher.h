#ifndef _DISPATCHER_C__H
#define _DISPATCHER_C__H
#include "LiteC-core.h"
// core support and defined classes init
extern void __declareClasses();
// methods
enum _VERBS { //a symbol for each distinct method name
    isDirectory_ = -_CORE_METHODS_MAX-161,
    isFile_,
    option_,
    valueFor_,
    getPos_,
    search_,
    compile_,
    compileFile_,
    compileFileOnModule_,
    parseOnModule_,
    createNewModule_,
    produceModule_,
    importDependencies_,
    importModule_,
    getInterface_,
    compilerVar_,
    setCompilerVar_,
    redirectOutput_,
    lock_,
    getParent_,
    positionText_,
    sayErr_,
    warn_,
    throwError_,
    throwParseFailed_,
    parse_,
    produce_,
    parseDirect_,
    opt_,
    req_,
    reqOneOf_,
    optList_,
    optSeparatedList_,
    optFreeFormList_,
    reqSeparatedList_,
    listArgs_,
    out_,
    outSourceLineAsComment_,
    outSourceLinesAsComment_,
    getEOLComment_,
    addSourceMap_,
    levelIndent_,
    parseAccessors_,
    addAccessor_,
    hasAdjective_,
    parseType_,
    declareName_,
    addMemberTo_,
    tryGetMember_,
    getScopeNode_,
    findInScope_,
    tryGetFromScope_,
    addToScope_,
    addToSpecificScope_,
    createScope_,
    tryGetOwnerNameDecl_,
    callOnSubTree_,
    lastLineOf_,
    getOwnerPrefix_,
    assignIfUndefined_,
    produceExport_,
    reset_,
    initSource_,
    preParseSource_,
    checkTitleCode_,
    tokenize_,
    preprocessor_,
    process_,
    nextSourceLine_,
    replaceSourceLine_,
    parseTripleQuotes_,
    checkMultilineComment_,
    checkConditionalCompilation_,
    setPos_,
    posToString_,
    getPrevIndent_,
    consumeToken_,
    nextToken_,
    returnToken_,
    nextCODELine_,
    say_,
    throwErr_,
    splitExpressions_,
    dump_,
    tokenizeLine_,
    recognizeToken_,
    start_,
    setHeader_,
    put_,
    startNewLine_,
    ensureNewLine_,
    blankLine_,
    getResult_,
    close_,
    markSourceMap_,
    add_,
    sourceLocation_,
    generate_,
    parseList_,
    declare_,
    evaluateAssignments_,
    createNameDeclaration_,
    declareInScope_,
    getTypeFromAssignedValue_,
    validatePropertyAccess_,
    produceInMap_,
    tryGetReference_,
    getResultType_,
    getPrecedence_,
    declareIntoVar_,
    growExpressionTree_,
    prepareTempVar_,
    getValue_,
    forEach_,
    parseParametersAndBody_,
    addMethodToOwnerNameDecl_,
    createReturnType_,
    produceBody_,
    processAppendToExtends_,
    getNodeJSRequireFileRef_,
    setTypes_,
    setSubType_,
    processItems_,
    produceInstanceOfLoop_,
    validate_,
    getCompiledLines_,
    getCompiledText_,
    addToExport_,
    confirmExports_,
    normalize_,
    setMember_,
    findOwnMember_,
    ownMember_,
    getMemberCount_,
    replaceForward_,
    makePointTo_,
    originalDeclarationPosition_,
    caseMismatch_,
    addMember_,
    composedName_,
    info_,
    findMember_,
    hasProto_,
    getMembersFromObjProperties_,
    isInParents_,
    processConvertTypes_,
    convertType_,
    assignTypeFromValue_,
    assignTypebyNameAffinity_,
    checkSuperChainProperties_,
    outWithExtension_,
    searchModule_,
    startsWith_,
    endsWith_,
    trimRight_,
    trimLeft_,
    capitalized_,
    quoted_,
    rpad_,
    repeat_,
    remove_,
_LAST_VERB};
// propery names
enum _THINGS { //a symbol for each distinct property name
    mtime_= _CORE_PROPS_LENGTH,
    mode_,
    soft_,
    lastIndex_,
    items_,
    verboseLevel_,
    warningLevel_,
    comments_,
    target_,
    debugEnabled_,
    perf_,
    skip_,
    generateSourceMap_,
    single_,
    compileIfNewer_,
    browser_,
    es6_,
    defines_,
    includeDirs_,
    projectDir_,
    mainModuleName_,
    outDir_,
    storeMessages_,
    literalMap_,
    version_,
    now_,
    options_,
    moduleCache_,
    rootModule_,
    compilerVars_,
    main_,
    Producer_,
    recurseLevel_,
    filesProducedCount_,
    parent_,
    childs_,
    keyword_,
    type_,
    keyType_,
    itemType_,
    lexer_,
    lineInx_,
    sourceLineNum_,
    column_,
    indent_,
    locked_,
    extraInfo_,
    accessors_,
    executes_,
    hasSideEffects_,
    isMap_,
    scope_,
    skipSemiColon_,
    project_,
    filename_,
    lines_,
    infoLines_,
    line_,
    infoLine_,
    token_,
    index_,
    interfaceMode_,
    stringInterpolationChar_,
    last_,
    maxSourceLineNum_,
    hardError_,
    softError_,
    outCode_,
    text_,
    tokens_,
    pre_,
    section_,
    post_,
    postIndent_,
    lineNum_,
    currLine_,
    header_,
    fileMode_,
    filenames_,
    fileIsOpen_,
    fHandles_,
    lastOriginalCodeComment_,
    lastOutCommentLine_,
    exportNamespace_,
    orTempVarCount_,
    sourceMap_,
    col_,
    lin_,
    columns_,
    args_,
    list_,
    aliasVarRef_,
    assignedValue_,
    nameDecl_,
    declared_,
    varRef_,
    body_,
    exceptionBlock_,
    catchVar_,
    finallyBody_,
    specifier_,
    expr_,
    conditional_,
    elseStatement_,
    nextIf_,
    preWhileUntilExpression_,
    postWhileUntilExpression_,
    control_,
    variant_,
    indexVar_,
    mainVar_,
    iterable_,
    where_,
    conditionPrefix_,
    endExpression_,
    increment_,
    filterExpression_,
    lvalue_,
    rvalue_,
    preIncDec_,
    postIncDec_,
    expression_,
    negated_,
    left_,
    right_,
    pushed_,
    precedence_,
    intoVar_,
    operandCount_,
    root_,
    ternaryCount_,
    produceType_,
    paramsDeclarations_,
    definePropItems_,
    hasExceptionBlock_,
    EndFnLineNum_,
    varRefSuper_,
    toNamespace_,
    kind_,
    endLineInx_,
    global_,
    importParameter_,
    importedModule_,
    names_,
    globVar_,
    assignment_,
    references_,
    fnCall_,
    arrExpression_,
    isInstanceof_,
    cases_,
    elseBody_,
    expressions_,
    adjectives_,
    specific_,
    preParsedVarRef_,
    intoVars_,
    lastSourceLineNum_,
    statements_,
    isMain_,
    exportDefault_,
    fileInfo_,
    exports_,
    exportsReplaced_,
    requireCallNodes_,
    referenceCount_,
    members_,
    nodeDeclared_,
    normalizeModeKeepFirstCase_,
    isScope_,
    nodeClass_,
    isPublicVar_,
    isForward_,
    isDummy_,
    superDecl_,
    pointsTo_,
    returnType_,
    informError_,
    converted_,
    failures_,
    importInfo_,
    dir_,
    extension_,
    base_,
    sourcename_,
    hasPath_,
    isCore_,
    isLite_,
    isInterface_,
    relPath_,
    relFilename_,
    outFilename_,
    outRelFilename_,
    outExtension_,
    outFileIsNewer_,
    interfaceFile_,
    interfaceFileExists_,
    externalCacheExists_,
    source_,
    isGlobalDeclare_,
    globalImport_,
    createFile_,
_LAST_THING};
#include "c_lite.h"
#include "interfaces/C_standalone/fs.h"
#include "interfaces/C_standalone/path.h"
#include "lib/color.h"
#include "lib/ControlledError.h"
#include "lib/OptionsParser.h"
#include "lib/GeneralOptions.h"
#include "Compiler.h"
#include "Project.h"
#include "ASTBase.h"
#include "Parser.h"
#include "lib/logger.h"
#include "lib/mkPath.h"
#include "lib/SourceMap.h"
#include "Grammar.h"
#include "lib/UniqueID.h"
#include "Names.h"
#include "Validate.h"
#include "lib/Environment.h"
#include "lib/shims.h"
#include "Producer_js.h"
#endif