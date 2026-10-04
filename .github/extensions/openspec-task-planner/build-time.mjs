export function parseSlotMinutes(text) {
    const match = /^(\d{1,2})(?::(\d{2}))?\s*(AM|PM)$/i.exec((text ?? "").trim());
    if (!match || +match[1] < 1 || +match[1] > 12 || +(match[2] ?? 0) > 59) return null;
    return (+match[1] % 12 + (match[3].toUpperCase() === "PM" ? 12 : 0)) * 60 + +(match[2] ?? 0);
}

export function shipmentTiming(build, deliveredAt = null, timeZone = "America/Los_Angeles") {
    let date = build.date;
    let minutes = parseSlotMinutes(build.slot);
    const dayTime = Date.parse(date + "T00:00:00Z");
    if (!/^\d{4}-\d{2}-\d{2}$/.test(date ?? "") || !Number.isFinite(dayTime)
        || new Date(dayTime).toISOString().slice(0, 10) !== date || minutes === null) {
        return { date, time: null, label: "Invalid shipment date/time",
            dataError: "Shipment heading needs YYYY-MM-DD — h:mm AM/PM. Correct the build changelist." };
    }
    if (deliveredAt) {
        const timestamp = new Date(deliveredAt);
        if (!Number.isFinite(timestamp.getTime())) throw new Error("Invalid shipment timestamp.");
        const parts = new Intl.DateTimeFormat("en-US", { timeZone, year: "numeric", month: "2-digit",
            day: "2-digit", hour: "2-digit", minute: "2-digit", hourCycle: "h23" }).formatToParts(timestamp);
        const part = (type) => parts.find((value) => value.type === type).value;
        date = ["year", "month", "day"].map(part).join("-");
        minutes = +part("hour") * 60 + +part("minute");
    }
    const hour = Math.floor(minutes / 60);
    return { date, time: Date.parse(date + "T00:00:00Z") + minutes * 60_000,
        label: `${date} — ${hour % 12 || 12}:${String(minutes % 60).padStart(2, "0")} ${hour < 12 ? "AM" : "PM"}`,
        dataError: null };
}
