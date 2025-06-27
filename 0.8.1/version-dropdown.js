document.addEventListener("DOMContentLoaded", function() {
  // Find the element with id 'projectnumber'
  const targetElem = document.getElementById("projectnumber");
  if (!targetElem) return;

  // Create a dropdown (select element)
  const select = document.createElement("select");
  select.id = "version-dropdown";

  // Add a path suffix for correct work on GitHub Pages
  const pathSuffix = window.location.pathname.includes("uniot-core") ? "/uniot-core" : "";
  // Fetch versions.json from the root of your site
  fetch(`${pathSuffix}/versions.json`)
    .then(function(response) {
      if (!response.ok) {
        throw new Error("Failed to fetch versions.json");
      }
      return response.json();
    })
    .then(function(data) {
      // The JSON should have a key "versions" that is an array of version strings
      const versions = data.versions || [];
      // Include "latest" as the first option
      versions.unshift("latest");

      // Sort versions by numeric values, ignoring 'v' prefix and keeping 'latest' at the top
      versions.sort(function(a, b) {
        if (a === "latest") return -1;
        if (b === "latest") return 1;

        // Remove 'v' prefix if it exists
        const verA = a.startsWith('v') ? a.substring(1) : a;
        const verB = b.startsWith('v') ? b.substring(1) : b;

        // Split version strings into components and convert to numbers
        const partsA = verA.split('.').map(Number);
        const partsB = verB.split('.').map(Number);

        // Compare each component
        for (let i = 0; i < Math.max(partsA.length, partsB.length); i++) {
          const numA = i < partsA.length ? partsA[i] : 0;
          const numB = i < partsB.length ? partsB[i] : 0;

          if (numA !== numB) {
            return numB - numA; // Descending order (newer versions first)
          }
        }

        return 0;
      });

      // Populate the dropdown with options
      versions.forEach(function(version) {
        const option = document.createElement("option");
        option.value = version;
        option.textContent = version === "latest" ? "Latest" : version;
        // Set as selected if the current URL indicates this version
        const currentPath = window.location.pathname;
        if ((version === "latest" && currentPath.includes("/latest/")) ||
            (version !== "latest" && currentPath.includes("/" + version + "/"))) {
          option.selected = true;
        }
        select.appendChild(option);
      });

      // Replace the target element's content with the dropdown
      targetElem.innerHTML = "";
      targetElem.appendChild(select);

      // Add an event listener to handle version changes
      select.addEventListener("change", function() {
        const selectedVersion = this.value;
        // Construct the URL for the selected version's documentation
        const getNewUrl = (selectedVersion) => {
          const currentPath = window.location.pathname;
          const pathRegex = pathSuffix.length ? /\/uniot-core\/(?:[^/]+)(\/.*)/ : /\/(?:[^/]+)(\/.*)/;
          const match = currentPath.match(pathRegex);

          if (match && match[1]) {
            // If we can extract the path after the version, maintain it
            return `${pathSuffix}/${selectedVersion}${match[1]}`;
          } else {
            // Fallback to index page if we can't parse the current path
            return `${pathSuffix}/${selectedVersion}/index.html`;
          }
        };

        // Check if the target documentation exists by sending a HEAD request
        fetch(getNewUrl(selectedVersion), { method: "HEAD" })
          .then((response) => {
            if (response.ok) {
              window.location.href = getNewUrl(selectedVersion);
            } else {
              alert("Documentation for version " + selectedVersion + " is not available.");
              this.value = "latest";
              window.location.href = getNewUrl(this.value);
            }
          })
          .catch((error) => {
            alert("Documentation for version " + selectedVersion + " is not available.");
            this.value = "latest";
            window.location.href = getNewUrl(this.value);
          });
      });
    })
    .catch(function(error) {
      console.error("Error loading versions:", error);
    });
});
