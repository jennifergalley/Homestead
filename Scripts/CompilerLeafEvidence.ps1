function Assert-CompilerLeafAccounting($Job, [array]$Samples, [uint32]$RootPid, [string]$CompilerImage) {
    if ($Job.Flags -ne 8200 -or $Job.ProcessLimit -ne 1 -or $Job.ActiveProcesses -ne 0 -or
        $Job.TotalProcesses -lt 1 -or $Job.TotalProcesses -gt 2 -or -not $Samples.Count) {
        throw 'Compiler job contains unclassified process accounting; no helper-free execution admission.'
    }
    $seen=@{}
    foreach($sample in $Samples) {
        if($sample.Error) { throw "Job observation failed:$($sample.Error)" }
        foreach($member in $sample.Members) {
            $key=$member.Pid.ToString()
            if($member.Error -and $member.NativeError -eq 87 -and $seen.ContainsKey($key)) { continue }
            if($member.Error -or -not $member.CreationTime -or
                (-not $member.Member -and (-not $member.Exited -or -not $seen.ContainsKey($key)))) { throw 'Unverified job member.' }
            $expected=if($member.Pid -eq $RootPid){$CompilerImage}else{'C:\Windows\System32\conhost.exe'}
            if($member.Image -ine $expected) { throw "Unadmitted job image:$($member.Image)" }
            if($seen.ContainsKey($key) -and $seen[$key].CreationTime -ne $member.CreationTime) { throw 'Job PID identity changed.' }
            $seen[$key]=$member
        }
    }
    if($seen.Count -ne $Job.TotalProcesses -or -not $seen.ContainsKey($RootPid.ToString())) {
        throw 'A job member was not identified during its lifetime.'
    }
    foreach($member in $seen.Values) {
        if($member.CreationTime -lt $seen[$RootPid.ToString()].CreationTime) { throw 'Job helper predates its owned root.' }
    }
}

function Read-ResourceCoff([string]$Path) {
    $file=Get-Item -LiteralPath $Path
    if($file.Length -lt 60 -or $file.Length -gt 16MB){throw 'Resource COFF size outside bounds.'}
    $bytes=[IO.File]::ReadAllBytes($Path)
    $count=[BitConverter]::ToUInt16($bytes,2)
    if([BitConverter]::ToUInt16($bytes,0) -ne 0x8664 -or $count -lt 1 -or $count -gt 32 -or
        20+40*$count -gt $bytes.Length -or [BitConverter]::ToUInt16($bytes,16) -ne 0){throw 'Invalid AMD64 resource COFF header.'}
    $sections=@()
    for($i=0;$i -lt $count;$i++) {
        $at=20+40*$i;$name=[Text.Encoding]::ASCII.GetString($bytes,$at,8).TrimEnd([char]0)
        $size=[uint64][BitConverter]::ToUInt32($bytes,$at+16);$offset=[uint64][BitConverter]::ToUInt32($bytes,$at+20)
        $flags=[BitConverter]::ToUInt32($bytes,$at+36)
        if($size -and ($offset -lt 20+40*$count -or $offset+$size -gt $bytes.Length)){throw 'Resource section exceeds object bounds.'}
        if($name.StartsWith('.rsrc') -and (($flags -band 0x40000000) -eq 0 -or ($flags -band 0x80000000L) -ne 0)){throw 'Resource section is not read-only.'}
        $sections+=@{name=$name;bytes=$size;flags=$flags}
    }
    if(-not @($sections|Where-Object name -Like '.rsrc*').Count){throw 'No actual resource section found.'}
    [pscustomobject]@{machine='AMD64';bytes=$bytes.Length;sections=$sections;sha256=(Get-FileHash $Path).Hash;codeExecuted=$false}
}

function Read-CompilerLeafCoff([byte[]]$Bytes) {
    if ($Bytes.Length -lt 20 -or $Bytes.Length -gt 1MB) { throw 'COFF size outside fixture bounds.' }
    $machine = [BitConverter]::ToUInt16($Bytes,0)
    $count = [BitConverter]::ToUInt16($Bytes,2)
    $symbols = [uint64][BitConverter]::ToUInt32($Bytes,8)
    $symbolCount = [uint64][BitConverter]::ToUInt32($Bytes,12)
    $optional = [BitConverter]::ToUInt16($Bytes,16)
    if ($machine -ne 0x8664 -or $count -lt 1 -or $count -gt 64 -or $optional -ne 0 -or
        20+40*$count -gt $Bytes.Length -or $symbolCount -lt 1 -or $symbolCount -gt 10000 -or
        $symbols -lt 20+40*$count -or $symbols+18*$symbolCount+4 -gt $Bytes.Length) {
        throw 'Invalid AMD64 COFF fixture header.'
    }
    $strings = $symbols+18*$symbolCount
    $stringBytes = [uint64][BitConverter]::ToUInt32($Bytes,[int]$strings)
    if ($stringBytes -lt 4 -or $strings+$stringBytes -gt $Bytes.Length) { throw 'Invalid COFF string table.' }
    $sections = @()
    for ($index=0;$index -lt $count;$index++) {
        $at=20+40*$index
        $name=[Text.Encoding]::ASCII.GetString($Bytes,$at,8).TrimEnd([char]0)
        $length=[uint64][BitConverter]::ToUInt32($Bytes,$at+16)
        $raw=[uint64][BitConverter]::ToUInt32($Bytes,$at+20)
        $flags=[BitConverter]::ToUInt32($Bytes,$at+36)
        if ($length -and ($raw -lt 20+40*$count -or $raw+$length -gt $symbols)) { throw 'Invalid section extent.' }
        $sections += [pscustomobject]@{name=$name;bytes=$length;offset=$raw;flags=$flags}
    }
    $found=$null
    for ($index=0;$index -lt $symbolCount;) {
        $at=[int]($symbols+18*$index)
        if ([BitConverter]::ToUInt32($Bytes,$at) -eq 0) {
            $offset=[uint64][BitConverter]::ToUInt32($Bytes,$at+4)
            if ($offset -lt 4 -or $offset -ge $stringBytes) { throw 'Invalid symbol-name offset.' }
            $end=[int]($strings+$offset)
            while ($end -lt $strings+$stringBytes -and $Bytes[$end] -ne 0) { $end++ }
            if ($end -eq $strings+$stringBytes) { throw 'Unterminated symbol name.' }
            $name=[Text.Encoding]::ASCII.GetString($Bytes,[int]($strings+$offset),[int]($end-$strings-$offset))
        } else { $name=[Text.Encoding]::ASCII.GetString($Bytes,$at,8).TrimEnd([char]0) }
        $aux=$Bytes[$at+17]
        if ($index+1+$aux -gt $symbolCount) { throw 'Invalid auxiliary-symbol count.' }
        if ($name -ceq 'HomesteadCompilerLeafFixture') {
            if ($found) { throw 'Duplicate fixture definition.' }
            $section=[BitConverter]::ToInt16($Bytes,$at+12)
            $value=[uint64][BitConverter]::ToUInt32($Bytes,$at+8)
            if ($section -lt 1 -or $section -gt $count -or $Bytes[$at+16] -ne 2 -or
                [BitConverter]::ToUInt16($Bytes,$at+14) -ne 0x20) { throw 'Invalid external function symbol.' }
            $data=$sections[$section-1]
            if (($data.flags -band 0x20000020) -ne 0x20000020 -or $value+6 -gt $data.bytes) { throw 'Invalid code section.' }
            $code=[Convert]::ToHexString($Bytes,[int]($data.offset+$value),6)
            if ($code -cne 'B8DF9B5713C3') { throw 'Fixture code is not mov eax,13579BDFh; ret.' }
            $found=[pscustomobject]@{symbol=$name;section=$data.name;offset=$value;machineCode=$code;returnValue=0x13579BDF}
        }
        $index+=1+$aux
    }
    if (-not $found -or -not ($sections | Where-Object name -EQ '.debug$S')) { throw 'Missing fixture definition or Z7 debug section.' }
    [pscustomobject]@{machine='AMD64';bytes=$Bytes.Length;sections=$sections;symbolCount=$symbolCount;function=$found;codeExecuted=$false}
}
