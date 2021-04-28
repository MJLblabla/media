/*
 * Copyright 2021 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
package com.google.android.exoplayer2;

import static com.google.android.exoplayer2.util.Assertions.checkArgument;

import android.os.Bundle;
import androidx.annotation.IntDef;
import androidx.annotation.Nullable;
import com.google.common.base.Objects;
import java.lang.annotation.Documented;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * A class for rating with a single degree of rating, "heart" vs "no heart". This can be used to
 * indicate the content referred to is a favorite (or not).
 */
public final class HeartRating extends Rating {

  @RatingType private static final int TYPE = RATING_TYPE_HEART;

  private final boolean isRated;

  /** Whether the rating has a heart rating or not. */
  public final boolean hasHeart;

  /** Creates a unrated HeartRating instance. */
  public HeartRating() {
    isRated = false;
    hasHeart = false;
  }

  /**
   * Creates a HeartRating instance.
   *
   * @param hasHeart true for a "heart selected" rating, false for "heart unselected".
   */
  public HeartRating(boolean hasHeart) {
    isRated = true;
    this.hasHeart = hasHeart;
  }

  @Override
  public boolean isRated() {
    return isRated;
  }

  @Override
  public int hashCode() {
    return Objects.hashCode(isRated, hasHeart);
  }

  @Override
  public boolean equals(@Nullable Object obj) {
    if (!(obj instanceof HeartRating)) {
      return false;
    }
    HeartRating other = (HeartRating) obj;
    return hasHeart == other.hasHeart && isRated == other.isRated;
  }

  @Override
  public String toString() {
    return "HeartRating: " + (isRated ? "hasHeart=" + hasHeart : "unrated");
  }

  // Bundleable implementation.
  @Documented
  @Retention(RetentionPolicy.SOURCE)
  @IntDef({FIELD_RATING_TYPE, FIELD_IS_RATED, FIELD_HAS_HEART})
  private @interface FieldNumber {}

  private static final int FIELD_IS_RATED = 1;
  private static final int FIELD_HAS_HEART = 2;

  @Override
  public Bundle toBundle() {
    Bundle bundle = new Bundle();
    bundle.putInt(keyForField(FIELD_RATING_TYPE), TYPE);
    bundle.putBoolean(keyForField(FIELD_IS_RATED), isRated);
    bundle.putBoolean(keyForField(FIELD_HAS_HEART), hasHeart);
    return bundle;
  }

  public static final Creator<HeartRating> CREATOR = HeartRating::fromBundle;

  private static HeartRating fromBundle(Bundle bundle) {
    checkArgument(
        bundle.getInt(keyForField(FIELD_RATING_TYPE), /* defaultValue= */ RATING_TYPE_DEFAULT)
            == TYPE);
    boolean isRated = bundle.getBoolean(keyForField(FIELD_IS_RATED), /* defaultValue= */ false);
    return isRated
        ? new HeartRating(
            bundle.getBoolean(keyForField(FIELD_HAS_HEART), /* defaultValue= */ false))
        : new HeartRating();
  }

  private static String keyForField(@FieldNumber int field) {
    return Integer.toString(field, Character.MAX_RADIX);
  }
}
