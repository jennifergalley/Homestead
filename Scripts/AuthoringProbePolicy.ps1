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
