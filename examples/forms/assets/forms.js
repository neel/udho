document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("json-form");
    if (!form) return;

    const body = document.getElementById("json-body");
    const output = document.getElementById("json-result");

    form.addEventListener("submit", async (event) => {
        event.preventDefault();
        output.textContent = "Sending JSON...";

        try {
            const response = await fetch(form.action, {
                method: "POST",
                headers: { "Content-Type": "application/json" },
                body: body.value
            });
            const data = await response.json();
            output.textContent = response.ok
                ? JSON.stringify(data, null, 2)
                : data.error || "The JSON request failed.";
        } catch (error) {
            output.textContent = "Could not send JSON: " + error.message;
        }
    });
});
