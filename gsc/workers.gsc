init_() {
    _SetupWorkers();
}

_SetupWorkers() {
    self endon("disconnect");
    for (i = 0; i < 10; i++) {
        setdvar("bo1zt_gsc_worker_" + i, "");
        self thread _RunWorker(i);         
    }
}

_RunWorker(workerId) {
    self endon("disconnect");
    while (1) {
        request = getdvar("bo1zt_gsc_worker_" + workerId);
        if (request != "") {
            requestTokens = strTok(request, "::");
            method = requestTokens[1];
            args = strTok(requestTokens[2], ",");
            result = "success";
            switch (method) {
                case "AddPerks":
                    bo1zt\gsc\api\perks::AddPerks(args);
                    break;
                case "RemovePerks":
                    bo1zt\gsc\api\perks::RemovePerks(args);
                    break;
                case "NumPerks":
                    result = bo1zt\gsc\api\perks::NumPerks();
                    break;
                case "StaticBox":
                    if (args.size == 0) {
                        result = bo1zt\gsc\api\static_box::GetStaticBox();
                    } else {
                        bo1zt\gsc\api\static_box::SetStaticBox(args);
                    }
                    break;
                case "PlayEasterEggSong":
                    result = bo1zt\gsc\api\music::PlayEasterEggSong();
                    break;
                default:
                    break;
            }
            setdvar("bo1zt_gsc_worker_" + workerId, "");
            wait 0.2; // Wait a bit for dvar update since its async
            self notify("bo1zt::Worker" + workerId + "::" + result);
        }
        wait 0.05;
    }
}
