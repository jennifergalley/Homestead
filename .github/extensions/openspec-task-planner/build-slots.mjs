// The planner has exactly two named build slots and no clock times: each build
// ships whenever its assigned work is done. Pure helpers only (no file access).

export const BUILD_SLOTS = Object.freeze([
    Object.freeze({ key: "next", label: "Next build" }),
    Object.freeze({ key: "after-next", label: "Build after next" }),
]);

const slotKeys = new Set(BUILD_SLOTS.map((slot) => slot.key));
const legacyDateTimeKey = /^\d{4}-\d{2}-\d{2} \d{2}:\d{2}$/;

export function isBuildSlotKey(value) {
    return slotKeys.has(value);
}

export function slotLabel(key) {
    return BUILD_SLOTS.find((slot) => slot.key === key)?.label ?? null;
}

// Planned cards in builds.md are headed exactly "Next build" / "Build after next".
export function slotKeyForHeading(heading) {
    const text = (heading ?? "").trim().toLowerCase();
    return BUILD_SLOTS.find((slot) => slot.label.toLowerCase() === text)?.key ?? null;
}

const isObject = (value) => value !== null && typeof value === "object" && !Array.isArray(value);
const strings = (value) => (Array.isArray(value) ? value.filter((item) => typeof item === "string") : []);

// Maps a priority document that may still carry fixed-time slot keys
// ("YYYY-MM-DD HH:mm") or legacy `nextBuild` IDs onto the two named slots.
// Distinct open date-times are ordered chronologically: the earliest becomes
// `next`, later ones `after-next`. A date-time that already shipped
// (`deliveredKeys`), or any other unrecognised key, was previously carried into
// the next open release, so it becomes `next`. Legacy `nextBuild` IDs without a
// slot become `next`. No assignment is dropped.
export function migrateSlots(priority, deliveredKeys = new Set()) {
    const source = isObject(priority) ? priority : {};
    const { nextBuild, ...rest } = source;
    const rawSlots = isObject(source.slots) ? source.slots : {};
    const open = [...new Set(Object.values(rawSlots).filter((value) => typeof value === "string"
        && legacyDateTimeKey.test(value) && !deliveredKeys.has(value)))].sort();
    const mapped = new Map(open.map((key, index) => [key, index === 0 ? "next" : "after-next"]));
    const slots = {};
    for (const [id, value] of Object.entries(rawSlots)) {
        if (typeof value !== "string" || !value) continue;
        slots[id] = isBuildSlotKey(value) ? value : mapped.get(value) ?? "next";
    }
    for (const id of strings(nextBuild)) if (!(id in slots)) slots[id] = "next";
    return { ...rest, order: strings(source.order), removed: strings(source.removed), slots };
}

// Integration's final step after a delivery: shipped IDs leave the slots,
// unfinished `next` items stay in `next`, and `after-next` items move up.
export function promoteAfterDelivery(priority, shippedIds, deliveredKeys = new Set()) {
    if (!Array.isArray(shippedIds) || shippedIds.some((id) => typeof id !== "string" || !id)) {
        throw new TypeError("shippedIds must be an array of non-empty ID strings.");
    }
    const shipped = new Set(shippedIds);
    const migrated = migrateSlots(priority, deliveredKeys);
    const slots = {};
    for (const [id, key] of Object.entries(migrated.slots)) {
        if (!shipped.has(id)) slots[id] = key === "after-next" ? "next" : key;
    }
    return { ...migrated, slots };
}
