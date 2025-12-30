init_() {
    
}

GetStaticBox() {        
    movable = getdvar("magic_chest_movable");
    if (movable == "1") {
        return "0";
    }
    return "1";
}

SetStaticBox(args) {        
    if (args.size >= 1 && args[0] == "1") {
        setdvar("magic_chest_movable", "0");
    } else {
        setdvar("magic_chest_movable", "1");
    }
    return "success";
}
