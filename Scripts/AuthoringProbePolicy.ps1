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
    $fileStores = 0
    foreach ($store in $Stores) {
        if ($store.type -eq '') { continue }
        if ($store.type -eq 'Memory') { continue }
        if ($store.type -ne 'File System' -or -not $store.local) { throw 'An effective DDC store is not admitted local file/memory storage.' }
        if ([IO.Path]::GetFullPath($store.name).TrimEnd('\') -ine [IO.Path]::GetFullPath($ExpectedPath).TrimEnd('\')) {
            throw 'Effective filesystem DDC path differs from the fresh probe cache.'
        }

        $fileStores++
    }
    if ($fileStores -ne 1) { throw 'Expected exactly one actual filesystem DDC store.' }
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
