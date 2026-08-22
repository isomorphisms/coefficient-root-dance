# F-Droid release path

This branch is stacked on `native-touch-prototype`, because that is where the Android application currently lives.

## Upstream release contract

1. Keep the Android `versionCode` and `versionName` source-controlled in `app/build.gradle.kts`.
2. Run the `F-Droid release build` workflow. It builds `assembleRelease`, checks the package/version and three native ABIs, and retains the unsigned APK as evidence.
3. Keep the repository license and F-Droid metadata aligned on `GPL-3.0-or-later`. `THIRD_PARTY.md` records reused and third-party material that is not silently relicensed by that grant.
4. After the native app lands on the release branch, tag the exact release commit `v<versionName>`.
5. Replace `FULL_COMMIT_HASH` in `org.isomorphisms.coefficientrootdance.yml.template` with the full hash of that tagged commit.
6. Copy the template to `fdroiddata/metadata/org.isomorphisms.coefficientrootdance.yml`, run `fdroid lint org.isomorphisms.coefficientrootdance`, then submit the fdroiddata merge request.

F-Droid rebuilds the app from source and signs the resulting APK itself. The upstream unsigned APK exists to prove that the same public source can produce a release package without private credentials.

After first inclusion, release tags can be picked up through `UpdateCheckMode: Tags` and `AutoUpdateMode: Version`.
