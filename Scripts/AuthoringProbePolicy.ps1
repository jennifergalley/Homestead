function Assert-AuthoringEndpoints {
    param([object[]]$Tcp, [object[]]$Udp, [uint32]$OwnedPid)
    if ($Udp.Count -ne 0 -or $Tcp.Count -gt 1) { throw 'Unexpected endpoint count/UDP in owned authoring leaf.' }
    foreach ($endpoint in $Tcp) {
        if ($endpoint.OwningProcess -ne $OwnedPid -or [int]$endpoint.State -ne 2 -or
            $endpoint.LocalAddress -ne '0.0.0.0' -or $endpoint.RemoteAddress -ne '0.0.0.0' -or
            $endpoint.RemotePort -ne 0 -or
            ($endpoint.LocalPort -ne 1985 -and ($endpoint.LocalPort -lt 32768 -or $endpoint.LocalPort -gt 40959))) {
            throw 'Owned TCP endpoint is not the admitted TraceControl listener.'
        }
    }
}

function Assert-AuthoringDdc {
    param([object[]]$Stores, [string]$ExpectedPath)
    if ($Stores.Count -lt 1 -or $Stores.Count -gt 128) { throw 'Invalid DDC traversal size.' }
    $fileStores = 0
    for ($index = 0; $index -lt $Stores.Count; $index++) {
        $store = $Stores[$index]
        if ($store.index -ne $index -or ($index -eq 0 -and $store.parent -ne -1) -or
            ($index -gt 0 -and ($store.parent -lt 0 -or $store.parent -ge $index)) -or
            $store.childCount -ne @($Stores | Where-Object parent -EQ $index).Count) {
            throw 'Malformed or incomplete DDC traversal.'
        }
        if ($store.type -in @('Async','')) {
            if ($store.name -ne '' -or $store.childCount -lt 1 -or
                ($store.type -eq 'Async' -and (-not $store.local -or $store.childCount -ne 1)) -or
                ($store.type -eq '' -and $store.local)) { throw 'Malformed DDC structural node.' }
            continue
        }
        if ($store.childCount -ne 0 -or -not $store.local) { throw 'Unexpected DDC storage children or remote store.' }
        if ($store.type -eq 'Memory' -and $store.name -eq '') { continue }
        if ($store.type -ne 'File System' -or -not [IO.Path]::IsPathFullyQualified($store.name)) {
            throw 'An effective DDC store is not admitted local file/memory storage.'
        }
        if ([IO.Path]::GetFullPath($store.name).TrimEnd('\') -ine [IO.Path]::GetFullPath($ExpectedPath).TrimEnd('\')) {
            throw 'Effective filesystem DDC path differs from the fresh probe cache.'
        }

        $fileStores++
    }
    if ($fileStores -ne 1) { throw 'Expected exactly one actual filesystem DDC store.' }
}

function Assert-AuthoringConfigBranches {
    param([object[]]$Branches, [string]$Output)
    $names = @('Engine','Editor','EditorSettings','EditorPerProjectUserSettings','GameUserSettings','Game','Input')
    if ($Branches.Count -ne $names.Count) { throw 'Config branch inventory is incomplete.' }
    foreach ($name in $names) {
        $matches = @($Branches | Where-Object name -CEQ $name)
        if ($matches.Count -ne 1) { throw 'Missing or duplicate config branch.' }
        $branch = $matches[0]
        if (-not $branch.found -or [string]::IsNullOrWhiteSpace($branch.logicalKey) -or
            -not [IO.Path]::IsPathFullyQualified($branch.destination) -or
            [IO.Path]::GetFullPath($branch.destination) -ine [IO.Path]::GetFullPath((Join-Path $Output "Config\$name.ini"))) {
            throw 'Actual config branch destination differs or is unavailable.'
        }
    }
}

function Assert-AuthoringPythonState {
    param($State)
    if (-not $State.valid -or -not $State.librariesEnumerated -or $State.available -or $State.initialized -or
        ($State.moduleLoaded -and -not $State.configured)) { throw 'Python public state is not disabled.' }
    foreach ($library in $State.libraries) {
        if ([IO.Path]::GetFileName($library.path) -notin @('python3.dll','python311.dll')) {
            throw 'Unexpected Python dependency library.'
        }
    }
    $runtime = @($State.libraries | Where-Object { [IO.Path]::GetFileName($_.path) -ieq 'python311.dll' })
    if ($State.runtimeLibraryLoaded) {
        if ($runtime.Count -ne 1 -or -not $runtime[0].queryAvailable -or $runtime[0].interpreterInitialized -ne 0) {
            throw 'Loaded CPython interpreter state is unavailable or initialized.'
        }
    } elseif ($State.moduleLoaded -or @($State.libraries).Count) {
        throw 'Python library absence is inconsistent with loaded modules.'
    }
}

function Assert-AuthoringRootObservation {
    param($Job, [object[]]$Members, $HeldRoot, [uint32]$OwnedPid,
        [string]$Image, [uint64]$CreationTime, [switch]$Final)
    if ($Job.Flags -ne 8200 -or $Job.ProcessLimit -ne 1 -or $Job.TotalProcesses -ne 1 -or
        $Job.ActiveProcesses -notin @(0,1) -or -not $Job.HeldProcessIsMember) {
        throw 'Authoring leaf policy or root-only total accounting differs.'
    }
    if ($HeldRoot.Error -or $HeldRoot.Pid -ne $OwnedPid -or $HeldRoot.Image -ine $Image -or
        $HeldRoot.CreationTime -ne $CreationTime -or -not $HeldRoot.IdentityFromHeldRoot -or
        (-not $HeldRoot.Member -and -not $HeldRoot.Exited)) {
        throw 'Retained authoring root identity/liveness differs.'
    }
    if ($Members.Count -gt 1) { throw 'An extra authoring job member executed.' }
    foreach ($member in $Members) {
        if ($member.Error -or $member.Pid -ne $OwnedPid -or $member.Image -ine $Image -or
            $member.CreationTime -ne $CreationTime -or (-not $member.Member -and -not $member.Exited)) {
            throw 'Unexpected or unidentified authoring job member.'
        }
    }
    if (($Job.ActiveProcesses -eq 0 -or $Members.Count -eq 0) -and -not $HeldRoot.Exited) {
        throw 'Missing live authoring root without observed process death.'
    }
    if ($Final -and ($Job.ActiveProcesses -ne 0 -or -not $HeldRoot.Exited -or $Members.Count -ne 0)) {
        throw 'Final authoring root death/empty job is not proved.'
    }
}
